"""Run the production P1 parser and merge logic on a host C++ compiler.
Usage: python test/host_power/run.py --compiler /path/to/zig.exe --build-dir /path/to/work
The Arduino String shim covers parsing only; UART/tasks are deliberately excluded.
Tests power regression, voltage validity, and strict CRC rejection.
"""
import argparse, os, pathlib, subprocess
p=argparse.ArgumentParser()
p.add_argument('--compiler',required=True)
p.add_argument('--build-dir',required=True)
a=p.parse_args()
folder=pathlib.Path(__file__).resolve().parent
root=folder.parent.parent
out=pathlib.Path(a.build_dir).resolve();out.mkdir(parents=True,exist_ok=True)
source=(root/'src/p1_reader.cpp').read_text(encoding='utf-8')
start=source.index('static float extractFloatValue(')
end=source.index('void P1Reader::p1ReaderTask(')
parser=out/'parser.cpp'
parser.write_text('#include "p1_reader.h"\n#include "p1_power_value.h"\nstruct SerialConsole { static void println(const String&) {} };\n'+source[start:end],encoding='utf-8')
exe=out/'test_power.exe'
command=[a.compiler]
if pathlib.Path(a.compiler).stem=='zig':command+=['c++']
command+=['-std=c++11','-Wall','-Wextra','-I'+str(folder),'-I'+str(root/'src'),str(parser),str(folder/'test_power.cpp'),'-o',str(exe)]
env=os.environ.copy()
if pathlib.Path(a.compiler).stem=='zig':
    env.setdefault('ZIG_GLOBAL_CACHE_DIR',str(out/'zig-global-cache'))
    env.setdefault('ZIG_LOCAL_CACHE_DIR',str(out/'zig-local-cache'))
subprocess.run(command,check=True,env=env)
subprocess.run([str(exe)],check=True)
