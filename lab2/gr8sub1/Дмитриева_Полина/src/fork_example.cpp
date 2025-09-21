#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <cstdlib>

int main(){
std::cout<<"Parent pro# cess started (PID="<<getpid()<<")"<<std::endl;

pid_t child_a = fork();

if(child_a==-1){
std::cerr<<"First fork failed!"<<std::endl;
exit(1);
}
else if(child_a==0){
std::cout<<"Child_A:My PID is "<<getpid()<<",My Parent`s PID is "<<getppid()<<std::endl;
exit(0);
}

pid_t child_b=fork();

if(child_b==-1){
std::cerr<<"Second fork failed!"<<std::endl;
exit(1);
}
else if(child_b==0){
std::cout<<"Child_B: My PID is "<<getpid()<<",My Parent`s PID is "<<getppid()<<std::endl;
exit(0);
}
int status;
waitpid(child_a,&status,0);
std::cout<<"Parent: Child A(PID="<<child_a<<") finished with status: "<<WEXITSTATUS(status)<<std::endl;

waitpid(child_b,&status,0);
std::cout<<"Parent: Child B (PID="<<child_b<<") finished with status: "<<WEXITSTATUS(status)<<std::endl;

std::cout<<"Parent: All children finished. Exiting."<<std::endl;

return 0;
}
