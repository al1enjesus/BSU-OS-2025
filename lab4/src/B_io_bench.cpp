#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>
#include <vector>
#include <cmath>

using namespace std;

static double
now_sec ()
{
  timespec ts{};
  clock_gettime (CLOCK_MONOTONIC, &ts);
  return ts.tv_sec + ts.tv_nsec * 1e-9;
}

static double
write_stdio_char (const string &path, size_t total_bytes, size_t &calls)
{
  FILE *f = fopen (path.c_str (), "wb");
  if (!f)
    {
      perror ("fopen");
      return NAN;
    }
  double t0 = now_sec ();
  calls = 0;
  for (size_t i = 0; i < total_bytes; ++i)
    {
      fputc ('A', f);
      ++calls;
    }
  fflush (f);
  fclose (f);
  return now_sec () - t0;
}

static double
write_stdio_buf (const string &path, size_t total_bytes, size_t bufsize,
                 size_t &calls)
{
  FILE *f = fopen (path.c_str (), "wb");
  if (!f)
    {
      perror ("fopen");
      return NAN;
    }
  vector<char> buf (bufsize, 'A');
  vector<char> iobuf (bufsize);
  setvbuf (f, iobuf.data (), _IOFBF, iobuf.size ());

  double t0 = now_sec ();
  size_t done = 0;
  calls = 0;
  while (done < total_bytes)
    {
      size_t n = min (bufsize, total_bytes - done);
      fwrite (buf.data (), 1, n, f);
      done += n;
      ++calls;
    }
  fflush (f);
  fclose (f);
  return now_sec () - t0;
}

static double
write_sys (const string &path, size_t total_bytes, size_t bufsize,
           size_t &calls)
{
  int fd = ::open (path.c_str (), O_WRONLY | O_CREAT | O_TRUNC, 0644);
  if (fd < 0)
    {
      perror ("open");
      return NAN;
    }
  vector<char> buf (bufsize, 'A');

  double t0 = now_sec ();
  size_t done = 0;
  calls = 0;
  while (done < total_bytes)
    {
      size_t n = min (bufsize, total_bytes - done);
      ssize_t w = ::write (fd, buf.data (), n);
      if (w < 0)
        {
          perror ("write");
          break;
        }
      done += static_cast<size_t> (w);
      ++calls;
    }
  ::fsync (fd);
  ::close (fd);
  return now_sec () - t0;
}

static double
read_stdio (const string &path, size_t bufsize, size_t &calls)
{
  FILE *f = fopen (path.c_str (), "rb");
  if (!f)
    {
      perror ("fopen");
      return NAN;
    }
  vector<char> buf (bufsize);

  double t0 = now_sec ();
  calls = 0;
  while (true)
    {
      size_t n = fread (buf.data (), 1, buf.size (), f);
      if (n == 0)
        break;
      ++calls;
    }
  fclose (f);
  return now_sec () - t0;
}

static double
read_sys (const string &path, size_t bufsize, size_t &calls)
{
  int fd = ::open (path.c_str (), O_RDONLY);
  if (fd < 0)
    {
      perror ("open");
      return NAN;
    }
  vector<char> buf (bufsize);

  double t0 = now_sec ();
  calls = 0;
  while (true)
    {
      ssize_t n = ::read (fd, buf.data (), buf.size ());
      if (n <= 0)
        break;
      ++calls;
    }
  ::close (fd);
  return now_sec () - t0;
}

int
main (int argc, char **argv)
{
  size_t size_mb = 100;
  vector<size_t> bufs = { 512, 4096, 65536 };
  bool do_stdio_char = true, do_stdio_buf = true, do_sys = true;
  string base = "bench";

  auto parse_list = [] (const string &s) {
    vector<size_t> out;
    string tok;
    stringstream ss (s);
    while (getline (ss, tok, ','))
      if (!tok.empty ())
        out.push_back (stoul (tok));
    return out;
  };

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
              cerr << "Bad --size-mb value: '" << v << "' (" << e.what ()
                   << ")\n";
              return 2;
            }
        }
      else if (a.rfind ("--buffers=", 0) == 0)
        {
          size_t eq = a.find ('=');
          string v = (eq == string::npos) ? "" : a.substr (eq + 1);
          try
            {
              bufs = parse_list (v);
              if (bufs.empty ())
                throw invalid_argument ("empty buffers");
            }
          catch (const exception &e)
            {
              cerr << "Bad --buffers list: '" << v << "' (" << e.what ()
                   << ")\n";
              return 2;
            }
        }
      else if (a == "--no-stdio-char")
        {
          do_stdio_char = false;
        }
      else if (a == "--no-stdio-buf")
        {
          do_stdio_buf = false;
        }
      else if (a == "--no-sys")
        {
          do_sys = false;
        }
      else if (a.rfind ("--out=", 0) == 0)
        {
          base = a.substr (6);
        }
      else
        {
          cerr << "Unknown arg: " << a << "\n";
          return 2;
        }
    }

  if (size_mb == 0)
    {
      cerr << "Refusing to run with --size-mb=0 (no work to do)\n";
      return 2;
    }
  if (!do_stdio_char && !do_stdio_buf && !do_sys)
    {
      cerr << "All modes disabled.\n";
      return 2;
    }

  const size_t total = size_mb * 1024ull * 1024ull;

  cout << "mode,buf,size_mb,write_sec,read_sec,write_calls,read_calls\n";

  if (do_stdio_char)
    {
      string path = base + "_stdio_char.bin";
      size_t wc = 0;
      double w = write_stdio_char (path, total, wc);
      size_t rc = 0;
      double r = read_stdio (path, 4 * 1024, rc);
      cout << "stdio_char,1," << size_mb << "," << w << "," << r << "," << wc
           << "," << rc << "\n";
      unlink (path.c_str ());
    }

  if (do_stdio_buf)
    {
      for (auto b : bufs)
        {
          string path = base + "_stdio_" + to_string (b) + ".bin";
          size_t wc = 0;
          double w = write_stdio_buf (path, total, b, wc);
          size_t rc = 0;
          double r = read_stdio (path, b, rc);
          cout << "stdio_buf," << b << "," << size_mb << "," << w << "," << r
               << "," << wc << "," << rc << "\n";
          unlink (path.c_str ());
        }
    }

  if (do_sys)
    {
      for (auto b : bufs)
        {
          string path = base + "_sys_" + to_string (b) + ".bin";
          size_t wc = 0;
          double w = write_sys (path, total, b, wc);
          size_t rc = 0;
          double r = read_sys (path, b, rc);
          cout << "sys," << b << "," << size_mb << "," << w << "," << r << ","
               << wc << "," << rc << "\n";
          unlink (path.c_str ());
        }
    }

  return 0;
}
