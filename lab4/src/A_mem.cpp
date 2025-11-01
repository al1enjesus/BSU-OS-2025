#include <cctype>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <new>
#include <sstream>
#include <string>
#include <vector>

#include <sys/mman.h>
#include <unistd.h>

using namespace std;

struct MemStats
{
  long vm_size = 0;
  long vm_rss = 0;
  long pss = 0;
  long priv_clean = 0;
  long priv_dirty = 0;
  long
  uss () const
  {
    return priv_clean + priv_dirty;
  }
};

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
      if (isdigit (static_cast<unsigned char> (ch)))
        {
          got = true;
          v = v * 10 + (ch - '0');
        }
      else if (got)
        break;
    }
  return v; // kB
}

static MemStats
read_status_and_smaps ()
{
  MemStats ms;
  ifstream st ("/proc/self/status");
  string line;
  while (getline (st, line))
    {
      if (line.rfind ("VmSize:", 0) == 0)
        ms.vm_size = read_kb_after_colon (line);
      else if (line.rfind ("VmRSS:", 0) == 0)
        ms.vm_rss = read_kb_after_colon (line);
    }
  ifstream sr ("/proc/self/smaps_rollup");
  while (getline (sr, line))
    {
      if (line.rfind ("Pss:", 0) == 0)
        ms.pss = read_kb_after_colon (line);
      else if (line.rfind ("Private_Clean:", 0) == 0)
        ms.priv_clean = read_kb_after_colon (line);
      else if (line.rfind ("Private_Dirty:", 0) == 0)
        ms.priv_dirty = read_kb_after_colon (line);
    }
  return ms;
}

static void
print_maps_snapshot (const string &path = "/proc/self/maps")
{
  ifstream f (path);
  cout << "===== /proc/self/maps =====\n";
  string s;
  while (getline (f, s))
    cout << s << '\n';
  cout << "===== end maps =====\n";
}

static void
touch_pages (char *ptr, size_t bytes)
{
  const size_t page = static_cast<size_t> (sysconf (_SC_PAGESIZE));
  for (size_t i = 0; i < bytes; i += page)
    ptr[i] = static_cast<char> (i);
  if (bytes > 0)
    ptr[bytes - 1] = 1;
}

static void
print_stats (const string &title, const MemStats &ms)
{
  cout << "--- " << title << " ---\n";
  cout << "VmSize: " << ms.vm_size << " kB\n";
  cout << "VmRSS : " << ms.vm_rss << " kB\n";
  cout << "PSS   : " << ms.pss << " kB\n";
  cout << "USS   : " << ms.uss () << " kB (Private_Clean " << ms.priv_clean
       << ", Private_Dirty " << ms.priv_dirty << ")\n";
}

int
main (int argc, char **argv)
{
  size_t size_mb = 1;
  bool show_maps = false;
  for (int i = 1; i < argc; ++i)
    {
      string a = argv[i];
      if (a.rfind ("--size-mb=", 0) == 0)
        {
          size_t eq = a.find ('=');
          string v = (eq == string::npos) ? "" : a.substr (eq + 1);
          try
            {
              if (v.empty ())
                throw invalid_argument ("empty");
              size_mb = stoul (v);
            }
          catch (const exception &e)
            {
              cerr << "Bad argument for --size-mb: '" << v << "' (" << e.what ()
                   << ")\n";
              return 2;
            }
        }
      else if (a == "--show-maps")
        {
          show_maps = true;
        }
    }
  const size_t sz = size_mb * 1024ull * 1024ull;

  volatile char stack_var[1024];
  for (int i = 0; i < 1024; ++i)
    ((volatile char *)stack_var)[i] = static_cast<char> (i);

  auto before = read_status_and_smaps ();
  print_stats ("Before allocations", before);

  char *heap_var = new (nothrow) char[sz];
  if (!heap_var)
    {
      cerr << "heap allocation failed\n";
      return 1;
    }
  touch_pages (heap_var, sz);

  void *mmap_var = mmap (nullptr, sz, PROT_READ | PROT_WRITE,
                         MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
  if (mmap_var == MAP_FAILED)
    {
      cerr << "mmap failed\n";
      delete[] heap_var;
      return 1;
    }
  touch_pages (static_cast<char *> (mmap_var), sz);

  auto after = read_status_and_smaps ();
  print_stats ("After allocations (touched)", after);

  cout << fixed << setprecision (2);
  cout << "\nΔVmRSS: " << (after.vm_rss - before.vm_rss) << " kB for "
       << size_mb << " MB heap+mmap touched\n";
  cout << "ΔPSS  : " << (after.pss - before.pss) << " kB\n";
  cout << "ΔUSS  : " << (after.uss () - before.uss ()) << " kB\n";

  if (show_maps)
    print_maps_snapshot ();

  cerr << "PID: " << getpid () << ". Press ENTER to exit..." << endl;
  cin.get ();

  munmap (mmap_var, sz);
  delete[] heap_var;
  return 0;
}
