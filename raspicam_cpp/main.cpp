#include "cam.h"
#include <iostream>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <chrono>

using namespace std;

void takePic (char* filename)
{
	int pid, status;
	if((pid = fork()) == 0)
	{
		execl("/usr/bin/raspistill", "raspistill", "-t", "75", "-w", "1024", "-h", "768", "-o", filename, (char *)NULL);
	}
	waitpid(pid, &status, 0);
}

int main (void) 
{
	auto start = std::chrono::steady_clock::now();

    //Input your desired filepath in place of the filepath denoted here
	char filename[] = "test.jpg";
	takePic(filename);
	cout<<"Image successful.."<<endl;

	auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start);
    std::cout << "Time taken by function takePic: " << duration.count() << " milliseconds" << std::endl;

	return 0;
}