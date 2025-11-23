#include <dirent.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <unistd.h>

using namespace std;

struct DevStat
{
  string name;
  unsigned long long reads = 0, rmerge = 0, rsectors = 0, rms = 0;
  unsigned long long writes = 0, wmerge = 0, wsectors = 0, wms = 0;
  unsigned long long inflight = 0, ioms = 0, wioms = 0;
};

static map<string, DevStat>
read_diskstats ()
{
  ifstream f ("/proc/diskstats");
  map<string, DevStat> m;
  string line;
  while (getline (f, line))
    {
      if (line.empty ())
        continue;
      stringstream ss (line);
      int major = 0, minor = 0;
      DevStat d;
      ss >> major >> minor >> d.name >> d.reads >> d.rmerge >> d.rsectors
          >> d.rms >> d.writes >> d.wmerge >> d.wsectors >> d.wms >> d.inflight
          >> d.ioms >> d.wioms;
      if (!d.name.empty ())
        m[d.name] = d;
    }
  return m;
}

static string
read_scheduler (const string &dev)
{
  string p = string ("/sys/block/") + dev + "/queue/scheduler";
  ifstream f (p);
  string s;
  getline (f, s);
  return s.empty () ? "?" : s;
}

static void
print_proc_io (pid_t pid)
{
  string p = string ("/proc/") + to_string (pid) + "/io";
  ifstream f (p);
  if (!f)
    {
      cerr << "[warn] cannot open " << p << "\n";
      return;
    }
  cout << "--- /proc/" << pid << "/io ---\n";
  string s;
  while (getline (f, s))
    cout << s << '\n';
}

int
main (int argc, char **argv)
{
  int iters = 10;
  double interval = 1.0;
  string only_dev;
  pid_t pid = 0;

  for (int i = 1; i < argc; ++i)
    {
      string a = argv[i];
      if (a.rfind ("--iters=", 0) == 0)
        {
          size_t eq = a.find ('=');
          string v = (eq == string::npos) ? "" : a.substr (eq + 1);
          try
            {
              if (v.empty ())
                throw invalid_argument ("empty");
              iters = stoi (v);
            }
          catch (const exception &e)
            {
              cerr << "Bad --iters: '" << v << "'\n";
              return 2;
            }
        }
      else if (a.rfind ("--interval=", 0) == 0)
        {
          size_t eq = a.find ('=');
          string v = (eq == string::npos) ? "" : a.substr (eq + 1);
          try
            {
              if (v.empty ())
                throw invalid_argument ("empty");
              interval = stod (v);
            }
          catch (const exception &e)
            {
              cerr << "Bad --interval: '" << v << "'\n";
              return 2;
            }
        }
      else if (a.rfind ("--dev=", 0) == 0)
        {
          size_t eq = a.find ('=');
          only_dev = (eq == string::npos) ? "" : a.substr (eq + 1);
        }
      else if (a.rfind ("--pid=", 0) == 0)
        {
          size_t eq = a.find ('=');
          string v = (eq == string::npos) ? "" : a.substr (eq + 1);
          try
            {
              if (v.empty ())
                throw invalid_argument ("empty");
              pid = static_cast<pid_t> (stoi (v));
            }
          catch (const exception &e)
            {
              cerr << "Bad --pid: '" << v << "'\n";
              return 2;
            }
        }
      else if (a == "--help" || a == "-h")
        {
          cerr << "Usage: ./C_io_monitor [--iters=N] [--interval=S] "
                  "[--dev=NAME] [--pid=PID]\n"
                  "  --iters     : number of samples (default 10)\n"
                  "  --interval  : seconds between samples (default 1.0)\n"
                  "  --dev       : filter by device name (e.g. nvme0n1)\n"
                  "  --pid       : also print /proc/<PID>/io at the end\n";
          return 0;
        }
      else
        {
          cerr << "Unknown arg: " << a << "\n";
          return 2;
        }
    }

  if (iters <= 0)
    {
      cerr << "iters must be > 0\n";
      return 2;
    }
  if (interval <= 0)
    {
      cerr << "interval must be > 0\n";
      return 2;
    }

  cout.setf (std::ios::fixed);
  cout << setprecision (1);
  cout << "time,dev,read_MBps,write_MBps,util_%,await_ms\n";

  auto prev = read_diskstats ();
  for (int t = 0; t < iters; ++t)
    {
      usleep ((useconds_t)(interval * 1e6));
      auto cur = read_diskstats ();
      for (auto &kv : cur)
        {
          const string &dev = kv.first;
          if (!only_dev.empty () && dev != only_dev)
            continue;
          if (!prev.count (dev))
            continue;

          auto &a = prev[dev];
          auto &b = kv.second;

          double rKB = (double)(b.rsectors - a.rsectors) * 0.5;
          double wKB = (double)(b.wsectors - a.wsectors) * 0.5;

          double rMBps = (rKB / 1024.0) / interval;
          double wMBps = (wKB / 1024.0) / interval;

          double util = (double)(b.ioms - a.ioms) / (10.0 * interval);

          unsigned long long dops = (b.reads - a.reads) + (b.writes - a.writes);
          double await_ms
              = (dops > 0)
                    ? (double)((b.rms - a.rms) + (b.wms - a.wms)) / (double)dops
                    : 0.0;

          cout << (t + 1) * interval << "," << dev << "," << rMBps << ","
               << wMBps << "," << util << "," << await_ms << "\n";
        }
      prev.swap (cur);
    }

  cout << "\nSchedulers:\n";
  if (DIR *dir = opendir ("/sys/block"))
    {
      if (dirent * de; true)
        {
          while ((de = readdir (dir)))
            {
              string n = de->d_name;
              if (n == "." || n == "..")
                continue;
              cout << n << ": " << read_scheduler (n) << "\n";
            }
        }
      closedir (dir);
    }

  if (pid > 0)
    print_proc_io (pid);

  return 0;
}
