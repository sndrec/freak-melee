#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <fstream>
#include <sstream>
#include "actual_source_tool.h"
static unsigned checks;
#define CHECK(x) do{++checks;if(!(x)){std::fprintf(stderr,"UI source %u: %s (%s)\n",__LINE__,#x,diagnostics.c_str());std::exit(1);}}while(0)
static std::string read(const std::string& path)
{std::ifstream file(path,std::ios::binary);std::ostringstream stream;stream<<file.rdbuf();return stream.str();}
int main(int argc,char** argv)
{
    if(argc!=3)return 1;std::string diagnostics;unsigned line=0;
    const std::string source=read(argv[2]);CHECK(!source.empty());
    const std::string accepted_path=std::string(argv[1])+"/behavior.fms",accepted=read(accepted_path);
    CHECK(!accepted.empty());
    CHECK(run_script_tool(argv[1],source,false,&diagnostics,&line));CHECK(read(accepted_path)==accepted);CHECK(line==0);
    const std::string invalid="fighter:\n    behavior independent\naction Failure:\n    on tick:\n        unknown.binding()\n";
    CHECK(!run_script_tool(argv[1],invalid,false,&diagnostics,&line));CHECK(line>0&&!diagnostics.empty());CHECK(read(accepted_path)==accepted);
    CHECK(!run_script_tool(argv[1],invalid,true,&diagnostics,&line));CHECK(read(accepted_path)==accepted);
    const std::string candidate=source+"\n# Actual Workshop source-only publish fixture.\n";
    CHECK(run_script_tool(argv[1],candidate,true,&diagnostics,&line));CHECK(read(accepted_path)==candidate);CHECK(line==0);
    WIN32_FIND_DATAA entry;std::string pattern=std::string(argv[1])+"/behavior.editor.*";HANDLE files=FindFirstFileA(pattern.c_str(),&entry);
    CHECK(files==INVALID_HANDLE_VALUE);if(files!=INVALID_HANDLE_VALUE)FindClose(files);
    std::printf("{\"status\":\"PASS actual Workshop source tool process\",\"checks\":%u,\"generated_c\":false,\"script_dll\":false}\n",checks);return 0;
}
