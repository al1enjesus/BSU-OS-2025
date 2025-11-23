#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <fstream>
#include <getopt.h>
#include <iomanip>
#include <iostream>
#include <limits.h>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include <vector>

using namespace std;

static bool
is_number (const string &s)
{
  if (s.empty ())
    return false;
  for (unsigned char c : s)
    if (!isdigit (c))
      return false;
  return true;
}

static bool
file_exists (const string &p)
{
  struct stat st
  {
  };
  return ::stat (p.c_str (), &st) == 0;
}

static string
read_file (const string &path)
{
  ifstream f (path);
  if (!f)
    return {};
  stringstream ss;
  ss << f.rdbuf ();
  return ss.str ();
}

static vector<string>
split_ws (const string &s)
{
  vector<string> out;
  string cur;
  for (char c : s)
    {
      if (isspace ((unsigned char)c))
        {
          if (!cur.empty ())
            {
              out.push_back (cur);
              cur.clear ();
            }
        }
      else
        cur.push_back (c);
    }
  if (!cur.empty ())
    out.push_back (cur);
  return out;
}

static long
read_kb_after_colon (const string &line)
{
  auto pos = line.find (':');
  if (pos == string::npos)
    return 0;
  string rest = line.substr (pos + 1);
  long v = 0;
  bool got = false;
  for (char ch : rest)
    {
      if (isdigit ((unsigned char)ch))
        {
          got = true;
          v = v * 10 + (ch - '0');
        }
      else if (got)
        break;
    }
  return v;
}

struct ProcMem
{
  long vm_size_kb = 0;
  long vm_rss_kb = 0;
  long pss_kb = 0;
  long priv_clean_kb = 0;
  long priv_dirty_kb = 0;
  long shared_clean_kb = 0;
  long shared_dirty_kb = 0;
  long
  uss_kb () const
  {
    return priv_clean_kb + priv_dirty_kb;
  }
  long
  shared_kb () const
  {
    return shared_clean_kb + shared_dirty_kb;
  }
};

struct PageFaults
{
  unsigned long long minflt = 0, majflt = 0;
};

struct Mapping
{
  unsigned long start = 0, end = 0;
  string perms, path;
  long size_kb = 0, rss_kb = 0, pss_kb = 0;
  long shared_clean_kb = 0, shared_dirty_kb = 0;
  long private_clean_kb = 0, private_dirty_kb = 0;
};

struct CategoryTotalsFull
{
  long size_kb = 0, rss_kb = 0, pss_kb = 0, priv_kb = 0, shared_kb = 0;
};

static ProcMem
read_proc_mem (pid_t pid)
{
  ProcMem ms;
  {
    ifstream st ("/proc/" + to_string (pid) + "/status");
    string line;
    while (getline (st, line))
      {
        if (line.rfind ("VmSize:", 0) == 0)
          ms.vm_size_kb = read_kb_after_colon (line);
        else if (line.rfind ("VmRSS:", 0) == 0)
          ms.vm_rss_kb = read_kb_after_colon (line);
      }
  }
  {
    ifstream sr ("/proc/" + to_string (pid) + "/smaps_rollup");
    string line;
    while (getline (sr, line))
      {
        if (line.rfind ("Pss:", 0) == 0)
          ms.pss_kb = read_kb_after_colon (line);
        else if (line.rfind ("Private_Clean:", 0) == 0)
          ms.priv_clean_kb = read_kb_after_colon (line);
        else if (line.rfind ("Private_Dirty:", 0) == 0)
          ms.priv_dirty_kb = read_kb_after_colon (line);
        else if (line.rfind ("Shared_Clean:", 0) == 0)
          ms.shared_clean_kb = read_kb_after_colon (line);
        else if (line.rfind ("Shared_Dirty:", 0) == 0)
          ms.shared_dirty_kb = read_kb_after_colon (line);
      }
  }
  return ms;
}

static PageFaults
read_page_faults (pid_t pid)
{
  PageFaults pf{};
  string s = read_file ("/proc/" + to_string (pid) + "/stat");
  if (s.empty ())
    return pf;
  auto r = s.rfind (')');
  if (r == string::npos)
    return pf;
  string tail = s.substr (r + 2);
  auto toks = split_ws (tail);
  try
    {
      if (toks.size () > 9)
        {
          pf.minflt = stoull (toks[7]);
          pf.majflt = stoull (toks[9]);
        }
    }
  catch (...)
    {
    }
  return pf;
}

static string
read_comm (pid_t pid)
{
  string s = read_file ("/proc/" + to_string (pid) + "/comm");
  if (!s.empty () && (s.back () == '\n' || s.back () == '\r'))
    s.pop_back ();
  if (s.empty ())
    {
      s = read_file ("/proc/" + to_string (pid) + "/cmdline");
      replace (s.begin (), s.end (), '\0', ' ');
    }
  return s;
}

static vector<Mapping>
read_maps (pid_t pid)
{
  vector<Mapping> out;
  string path = "/proc/" + to_string (pid) + "/maps";
  ifstream f (path);
  string line;
  while (getline (f, line))
    {
      unsigned long start = 0, end = 0;
      char perms[8] = { 0 };
      char rest[PATH_MAX] = { 0 };
      int n = sscanf (line.c_str (), "%lx-%lx %7s %*s %*s %*s %[^\n]", &start,
                      &end, perms, rest);
      Mapping m;
      m.start = start;
      m.end = end;
      m.perms = perms;
      if (n == 4)
        m.path = rest;
      out.push_back (move (m));
    }
  return out;
}

static void
enrich_with_smaps (pid_t pid, vector<Mapping> &maps)
{
  unordered_map<unsigned long, size_t> by_start;
  by_start.reserve (maps.size () * 2);
  for (size_t i = 0; i < maps.size (); ++i)
    if (maps[i].start)
      by_start[maps[i].start] = i;

  ifstream f ("/proc/" + to_string (pid) + "/smaps");
  if (!f)
    {
      for (auto &m : maps)
        if (m.size_kb == 0 && m.end >= m.start)
          m.size_kb = (long)((m.end - m.start) / 1024UL);
      return;
    }

  auto parse_header
      = [] (const string &s, unsigned long &a, unsigned long &b) -> bool {
    unsigned long start = 0, end = 0;
    if (sscanf (s.c_str (), "%lx-%lx %*s %*s %*s %*s %*[^ \n]", &start, &end)
        == 2)
      {
        a = start;
        b = end;
        return true;
      }
    return false;
  };
  auto is_kv
      = [] (const string &s) -> bool { return s.find (':') != string::npos; };

  string line;
  size_t current_idx = (size_t)-1;
  unsigned long hdr_start = 0, hdr_end = 0;

  while (getline (f, line))
    {
      unsigned long a = 0, b = 0;
      if (parse_header (line, a, b))
        {
          hdr_start = a;
          hdr_end = b;
          current_idx = (size_t)-1;

          if (auto it = by_start.find (hdr_start); it != by_start.end ())
            {
              size_t cand = it->second;
              if (maps[cand].end == hdr_end)
                {
                  current_idx = cand;
                }
              else
                {
                  size_t from = cand > 8 ? cand - 8 : 0;
                  size_t to = min (maps.size (), cand + 9);
                  for (size_t j = from; j < to; ++j)
                    {
                      if (maps[j].start == hdr_start && maps[j].end == hdr_end)
                        {
                          current_idx = j;
                          break;
                        }
                    }
                }
            }
          if (current_idx == (size_t)-1)
            {
              for (size_t j = 0; j < maps.size (); ++j)
                {
                  if (maps[j].start == hdr_start && maps[j].end == hdr_end)
                    {
                      current_idx = j;
                      break;
                    }
                }
            }
          if (current_idx != (size_t)-1)
            {
              Mapping &m = maps[current_idx];
              if (m.size_kb == 0 && hdr_end >= hdr_start)
                m.size_kb = (long)((hdr_end - hdr_start) / 1024UL);
            }
          continue;
        }

      if (!is_kv (line) || current_idx == (size_t)-1)
        continue;

      Mapping &m = maps[current_idx];
      if (line.rfind ("Size:", 0) == 0)
        m.size_kb = read_kb_after_colon (line);
      else if (line.rfind ("Rss:", 0) == 0)
        m.rss_kb = read_kb_after_colon (line);
      else if (line.rfind ("Pss:", 0) == 0)
        m.pss_kb = read_kb_after_colon (line);
      else if (line.rfind ("Shared_Clean:", 0) == 0)
        m.shared_clean_kb = read_kb_after_colon (line);
      else if (line.rfind ("Shared_Dirty:", 0) == 0)
        m.shared_dirty_kb = read_kb_after_colon (line);
      else if (line.rfind ("Private_Clean:", 0) == 0)
        m.private_clean_kb = read_kb_after_colon (line);
      else if (line.rfind ("Private_Dirty:", 0) == 0)
        m.private_dirty_kb = read_kb_after_colon (line);
    }

  for (auto &m : maps)
    if (m.size_kb == 0 && m.end >= m.start)
      m.size_kb = (long)((m.end - m.start) / 1024UL);
}

static bool
is_stack_name (const string &p)
{
  if (p == "[stack]")
    return true;
  if (p.rfind ("[stack:", 0) == 0)
    return true;
  return false;
}
static bool
is_heap_name (const string &p)
{
  return p == "[heap]";
}
static bool
is_anon (const string &p)
{
  if (p.empty ())
    return true;
  if (p[0] == '[')
    return !is_heap_name (p) && !is_stack_name (p);
  return false;
}
static bool
looks_like_lib (const string &p)
{
  if (p.empty () || p[0] == '[')
    return false;
  if (p.find ("/lib") != string::npos)
    return true;
  if (p.find (".so") != string::npos)
    return true;
  return false;
}

static map<string, CategoryTotalsFull>
categorize (const vector<Mapping> &maps)
{
  CategoryTotalsFull heap{}, stack{}, libs{}, anon{}, other{};
  for (auto &m : maps)
    {
      long priv = m.private_clean_kb + m.private_dirty_kb;
      long sh = m.shared_clean_kb + m.shared_dirty_kb;
      auto add = [&] (CategoryTotalsFull &c) {
        c.size_kb += m.size_kb;
        c.rss_kb += m.rss_kb;
        c.pss_kb += m.pss_kb;
        c.priv_kb += priv;
        c.shared_kb += sh;
      };
      if (is_heap_name (m.path))
        add (heap);
      else if (is_stack_name (m.path))
        add (stack);
      else if (looks_like_lib (m.path))
        add (libs);
      else if (is_anon (m.path))
        add (anon);
      else
        add (other);
    }
  map<string, CategoryTotalsFull> out;
  out["heap"] = heap;
  out["stack"] = stack;
  out["libs"] = libs;
  out["anonymous"] = anon;
  out["other"] = other;
  return out;
}

static map<string, CategoryTotalsFull>
aggregate_libs (const vector<Mapping> &maps)
{
  map<string, CategoryTotalsFull> agg;
  for (auto &m : maps)
    {
      if (!looks_like_lib (m.path))
        continue;
      auto &c = agg[m.path];
      long priv = m.private_clean_kb + m.private_dirty_kb;
      long sh = m.shared_clean_kb + m.shared_dirty_kb;
      c.size_kb += m.size_kb;
      c.rss_kb += m.rss_kb;
      c.pss_kb += m.pss_kb;
      c.priv_kb += priv;
      c.shared_kb += sh;
    }
  return agg;
}

static map<string, int>
scan_shared_process_counts ()
{
  map<string, int> cnt;
  DIR *d = opendir ("/proc");
  if (!d)
    return cnt;
  dirent *de;
  while ((de = readdir (d)))
    {
      string name = de->d_name;
      if (!is_number (name))
        continue;
      string mp = "/proc/" + name + "/maps";
      ifstream f (mp);
      if (!f)
        continue;
      set<string> seen;
      string line;
      while (getline (f, line))
        {
          unsigned long a, b;
          char perms[8] = { 0 };
          char pathbuf[PATH_MAX] = { 0 };
          int n = sscanf (line.c_str (), "%lx-%lx %7s %*s %*s %*s %[^\n]", &a,
                          &b, perms, pathbuf);
          if (n == 4)
            {
              string p = pathbuf;
              if (looks_like_lib (p))
                seen.insert (p);
            }
        }
      for (auto &p : seen)
        cnt[p]++;
    }
  closedir (d);
  return cnt;
}

static string
kb2hr (long kb)
{
  double m = kb / 1024.0;
  stringstream ss;
  if (m < 1024.0)
    ss << fixed << setprecision (1) << m << " MB";
  else
    ss << fixed << setprecision (2) << (m / 1024.0) << " GB";
  return ss.str ();
}

static void
print_single_report (pid_t pid, bool share_scan)
{
  if (!file_exists ("/proc/" + to_string (pid)))
    {
      cerr << "PID " << pid << " not found.\n";
      return;
    }
  string name = read_comm (pid);
  ProcMem m = read_proc_mem (pid);
  PageFaults pf = read_page_faults (pid);

  cout << "Process: " << (name.empty () ? "?" : name) << " (PID " << pid
       << ")\n";
  cout << "VSZ:  " << kb2hr (m.vm_size_kb) << "\n";
  cout << "RSS:  " << kb2hr (m.vm_rss_kb) << "\n";
  cout << "PSS:  " << kb2hr (m.pss_kb) << "\n";
  cout << "USS:  " << kb2hr (m.uss_kb ()) << "\n";
  cout << "Shared:  " << kb2hr (m.shared_kb ())
       << "    Private: " << kb2hr (m.uss_kb ()) << "\n";

  cout << "\nPage Faults:\n";
  cout << "  Minor: " << pf.minflt << "\n";
  cout << "  Major: " << pf.majflt << "\n";

  auto maps = read_maps (pid);
  enrich_with_smaps (pid, maps);
  auto cats = categorize (maps);

  cout << "\nMemory Map (categories):\n";
  for (auto &kv : cats)
    {
      const string &k = kv.first;
      const auto &c = kv.second;
      cout << "  " << setw (10) << left << k << "  size " << kb2hr (c.size_kb)
           << " | RSS " << kb2hr (c.rss_kb) << " | PSS " << kb2hr (c.pss_kb)
           << " | Private " << kb2hr (c.priv_kb) << " | Shared "
           << kb2hr (c.shared_kb) << "\n";
    }

  cout << "\nTop libraries (by shared KB):\n";
  auto libs = aggregate_libs (maps);
  vector<pair<string, CategoryTotalsFull> > libv (libs.begin (), libs.end ());
  sort (libv.begin (), libv.end (), [] (auto &a, auto &b) {
    return a.second.shared_kb > b.second.shared_kb;
  });
  int shown = 0;
  map<string, int> share_cnt;
  if (share_scan)
    share_cnt = scan_shared_process_counts ();
  for (auto &p : libv)
    {
      if (shown++ >= 15)
        break;
      int nshare = share_scan ? share_cnt[p.first] : -1;
      cout << "  " << p.first << "\n";
      cout << "     size " << kb2hr (p.second.size_kb) << " | PSS "
           << kb2hr (p.second.pss_kb) << " | Private "
           << kb2hr (p.second.priv_kb) << " | Shared "
           << kb2hr (p.second.shared_kb);
      if (share_scan)
        cout << " | used_by_procs " << nshare;
      cout << "\n";
    }
}

static void
ascii_graph (const vector<long> &rss_kb_series)
{
  if (rss_kb_series.empty ())
    return;
  long mn = *min_element (rss_kb_series.begin (), rss_kb_series.end ());
  long mx = *max_element (rss_kb_series.begin (), rss_kb_series.end ());
  int width = 60;
  if (const char *c = getenv ("COLUMNS"))
    {
      int w = atoi (c);
      if (w > 20)
        width = min (120, w - 20);
    }
  cout << "\nRSS ASCII graph (" << rss_kb_series.size ()
       << " samples, min=" << kb2hr (mn) << ", max=" << kb2hr (mx) << "):\n";
  if (mx == mn)
    {
      for (size_t i = 0; i < rss_kb_series.size (); ++i)
        cout << setw (4) << i << " | " << string (5, '#') << " "
             << kb2hr (rss_kb_series[i]) << "\n";
      return;
    }
  for (size_t i = 0; i < rss_kb_series.size (); ++i)
    {
      double t = double (rss_kb_series[i] - mn) / double (mx - mn);
      int bars = max (1, int (t * width));
      cout << setw (4) << i << " | " << string (bars, '#') << " "
           << kb2hr (rss_kb_series[i]) << "\n";
    }
}

static void
watch_mode (pid_t pid, int samples, double interval_sec, bool do_graph,
            const string &csv_path)
{
  if (!file_exists ("/proc/" + to_string (pid)))
    {
      cerr << "PID " << pid << " not found.\n";
      return;
    }
  string name = read_comm (pid);
  cout << "Watching: " << (name.empty () ? "?" : name) << " (PID " << pid
       << "), interval " << interval_sec << "s, samples " << samples << "\n\n";
  cout << left << setw (8) << "t(s)" << setw (12) << "RSS" << setw (12) << "PSS"
       << setw (12) << "USS" << setw (14) << "ΔRSS" << setw (14) << "ΔPSS"
       << setw (14) << "ΔUSS" << setw (16) << "minfltΔ" << setw (16)
       << "majfltΔ" << "\n";

  ofstream csv;
  if (!csv_path.empty ())
    {
      csv.open (csv_path);
      if (csv)
        csv << "t_sec,rss_kb,pss_kb,uss_kb,delta_rss_kb,delta_pss_kb,delta_uss_"
               "kb,delta_minflt,delta_majflt\n";
    }

  vector<long> rss_series;
  ProcMem prev_m = read_proc_mem (pid);
  PageFaults prev_pf = read_page_faults (pid);
  rss_series.push_back (prev_m.vm_rss_kb);

  cout << fixed << setprecision (1);
  cout << setw (8) << 0.0 << setw (12) << kb2hr (prev_m.vm_rss_kb) << setw (12)
       << kb2hr (prev_m.pss_kb) << setw (12) << kb2hr (prev_m.uss_kb ())
       << setw (14) << "+" << 0 << setw (14) << "+" << 0 << setw (14) << "+"
       << 0 << setw (16) << "+" << 0 << setw (16) << "+" << 0 << "\n";

  if (csv)
    csv << 0.0 << "," << prev_m.vm_rss_kb << "," << prev_m.pss_kb << ","
        << prev_m.uss_kb () << "," << 0 << "," << 0 << "," << 0 << "," << 0
        << "," << 0 << "\n";

  for (int i = 1; i < samples; ++i)
    {
      this_thread::sleep_for (chrono::duration<double> (interval_sec));
      if (!file_exists ("/proc/" + to_string (pid)))
        break;
      ProcMem cur_m = read_proc_mem (pid);
      PageFaults cur_pf = read_page_faults (pid);
      long d_rss = cur_m.vm_rss_kb - prev_m.vm_rss_kb;
      long d_pss = cur_m.pss_kb - prev_m.pss_kb;
      long d_uss = cur_m.uss_kb () - prev_m.uss_kb ();
      long d_min = (long)(cur_pf.minflt - prev_pf.minflt);
      long d_maj = (long)(cur_pf.majflt - prev_pf.majflt);

      double t = i * interval_sec;
      cout << setw (8) << t << setw (12) << kb2hr (cur_m.vm_rss_kb) << setw (12)
           << kb2hr (cur_m.pss_kb) << setw (12) << kb2hr (cur_m.uss_kb ())
           << setw (14) << (d_rss >= 0 ? "+" : "") << d_rss << "kB" << setw (14)
           << (d_pss >= 0 ? "+" : "") << d_pss << "kB" << setw (14)
           << (d_uss >= 0 ? "+" : "") << d_uss << "kB" << setw (16)
           << (d_min >= 0 ? "+" : "") << d_min << setw (16)
           << (d_maj >= 0 ? "+" : "") << d_maj << "\n";

      if (csv)
        csv << t << "," << cur_m.vm_rss_kb << "," << cur_m.pss_kb << ","
            << cur_m.uss_kb () << "," << d_rss << "," << d_pss << "," << d_uss
            << "," << d_min << "," << d_maj << "\n";

      prev_m = cur_m;
      prev_pf = cur_pf;
      rss_series.push_back (cur_m.vm_rss_kb);
    }

  if (do_graph)
    ascii_graph (rss_series);
  if (csv)
    {
      csv.close ();
      cout << "\nCSV saved to: " << csv_path << "\n";
    }
}

static void
compare_mode (pid_t p1, pid_t p2)
{
  if (!file_exists ("/proc/" + to_string (p1))
      || !file_exists ("/proc/" + to_string (p2)))
    {
      cerr << "Both PIDs must exist.\n";
      return;
    }
  string n1 = read_comm (p1), n2 = read_comm (p2);
  auto m1 = read_proc_mem (p1);
  auto m2 = read_proc_mem (p2);
  auto maps1 = read_maps (p1);
  enrich_with_smaps (p1, maps1);
  auto maps2 = read_maps (p2);
  enrich_with_smaps (p2, maps2);

  auto libs1 = aggregate_libs (maps1);
  auto libs2 = aggregate_libs (maps2);

  set<string> s1, s2;
  for (auto &x : libs1)
    s1.insert (x.first);
  for (auto &x : libs2)
    s2.insert (x.first);

  vector<string> inter, only1, only2;
  set_intersection (s1.begin (), s1.end (), s2.begin (), s2.end (),
                    back_inserter (inter));
  set_difference (s1.begin (), s1.end (), s2.begin (), s2.end (),
                  back_inserter (only1));
  set_difference (s2.begin (), s2.end (), s1.begin (), s1.end (),
                  back_inserter (only2));

  auto sum_pss = [] (const map<string, CategoryTotalsFull> &agg,
                     const vector<string> &keys) {
    long s = 0;
    for (auto &k : keys)
      {
        auto it = agg.find (k);
        if (it != agg.end ())
          s += it->second.pss_kb;
      }
    return s;
  };
  long inter_pss1 = sum_pss (libs1, inter);
  long inter_pss2 = sum_pss (libs2, inter);

  cout << "Compare processes:\n";
  cout << "  A: " << n1 << " (PID " << p1 << ")\n";
  cout << "     VSZ " << kb2hr (m1.vm_size_kb) << " | RSS "
       << kb2hr (m1.vm_rss_kb) << " | PSS " << kb2hr (m1.pss_kb) << " | USS "
       << kb2hr (m1.uss_kb ()) << "\n";
  cout << "  B: " << n2 << " (PID " << p2 << ")\n";
  cout << "     VSZ " << kb2hr (m2.vm_size_kb) << " | RSS "
       << kb2hr (m2.vm_rss_kb) << " | PSS " << kb2hr (m2.pss_kb) << " | USS "
       << kb2hr (m2.uss_kb ()) << "\n\n";

  cout << "Shared libraries intersection: " << inter.size () << "\n";
  cout << "  Estimated shared by PSS: " << kb2hr (min (inter_pss1, inter_pss2))
       << "\n\n";

  auto print_list = [&] (const string &title, const vector<string> &v,
                         const map<string, CategoryTotalsFull> &agg) {
    cout << title << " (" << v.size () << "):\n";
    int shown = 0;
    for (auto &k : v)
      {
        auto it = agg.find (k);
        if (it == agg.end ())
          continue;
        cout << "  " << k << " | PSS " << kb2hr (it->second.pss_kb)
             << " | Private " << kb2hr (it->second.priv_kb) << " | Shared "
             << kb2hr (it->second.shared_kb) << "\n";
        if (++shown >= 10)
          break;
      }
    if ((int)v.size () > shown)
      cout << "  ... (" << (v.size () - shown) << " more)\n";
    cout << "\n";
  };

  print_list ("Common libs (top 10)", inter, libs1);
  print_list ("Unique to A (top 10)", only1, libs1);
  print_list ("Unique to B (top 10)", only2, libs2);
}

static void
usage ()
{
  cerr << "Usage:\n"
          "  memory_profiler [options] PID\n"
          "  memory_profiler --watch [--interval=S] [--samples=N] [--graph] "
          "[--csv=FILE] PID\n"
          "  memory_profiler --compare PID1 PID2\n"
          "Options:\n"
          "  --watch           monitor periodically\n"
          "  --interval=S      sampling interval seconds (default 1.0)\n"
          "  --samples=N       number of samples (default 10)\n"
          "  --graph           ASCII graph of RSS at the end\n"
          "  --csv=FILE        write samples to CSV in watch mode\n"
          "  --share-scan      scan /proc to count how many processes map each "
          ".so\n"
          "  --compare         compare two processes\n";
}

int
main (int argc, char **argv)
{
  bool opt_watch = false, opt_graph = false, opt_compare_flag = false,
       opt_share_scan = false;
  double interval = 1.0;
  int samples = 10;
  string csv_path;

  static option longopts[] = { { "watch", no_argument, nullptr, 1 },
                               { "graph", no_argument, nullptr, 2 },
                               { "interval", required_argument, nullptr, 'i' },
                               { "samples", required_argument, nullptr, 'n' },
                               { "csv", required_argument, nullptr, 'c' },
                               { "compare", no_argument, nullptr, 3 },
                               { "share-scan", no_argument, nullptr, 4 },
                               { "help", no_argument, nullptr, 'h' },
                               { nullptr, 0, nullptr, 0 } };
  int opt, idx;
  while ((opt = getopt_long (argc, argv, "i:n:c:h", longopts, &idx)) != -1)
    {
      switch (opt)
        {
        case 1:
          opt_watch = true;
          break;
        case 2:
          opt_graph = true;
          break;
        case 3:
          opt_compare_flag = true;
          break;
        case 4:
          opt_share_scan = true;
          break;
        case 'i':
          interval = atof (optarg);
          break;
        case 'n':
          samples = atoi (optarg);
          break;
        case 'c':
          csv_path = optarg;
          break;
        case 'h':
          usage ();
          return 0;
        default:
          usage ();
          return 2;
        }
    }

  vector<string> args;
  for (int i = optind; i < argc; ++i)
    args.emplace_back (argv[i]);

  if (opt_compare_flag)
    {
      if (args.size () != 2 || !is_number (args[0]) || !is_number (args[1]))
        {
          usage ();
          return 2;
        }
      pid_t p1 = (pid_t)stol (args[0]);
      pid_t p2 = (pid_t)stol (args[1]);
      compare_mode (p1, p2);
      return 0;
    }

  if (args.size () != 1 || !is_number (args[0]))
    {
      usage ();
      return 2;
    }
  pid_t pid = (pid_t)stol (args[0]);

  if (opt_watch)
    {
      if (interval <= 0)
        interval = 1.0;
      if (samples <= 0)
        samples = 10;
      watch_mode (pid, samples, interval, opt_graph, csv_path);
    }
  else
    {
      print_single_report (pid, opt_share_scan);
    }
  return 0;
}
