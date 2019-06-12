#include <iostream>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <poll.h>
#include "List.h"

#define DURATION 5
#define DEFAULT_RIGHTS 0755

using namespace std;

List known;
void scanChanges(List& inserted, List& deleted);
char *commonDirectory;
char *mirrorDirectory;
char *inputDirectory;
int n, bufferSize;
char *logfile;

void handlerSIGALRM(int sig);
void handlerSIGINT(int sig);
void handlerSIGQUIT(int sig);
void handlerSIGCHLD(int sig);
void handlerSIGUSR1(int sig);
void handlerSIGUSR2(int sig);
void myrmDir(char* name);
char* createFifoName(int from, int to);
void createChilds(int j, int);
void createSubFolders(char* path);
void sendAllFiles(char* directory, int fd, int &error);
void sendFile(char* filename, int fd, int& error);
int writeAll(int size, int fd, char* buffer);
int readAll(int size, int fd, char* buffer);
int receiveFile(int fd, int j, int& error);
void receiveFiles(int fd, int j, int&);

int main(int argc, char **argv)
{
	cout << "PID = " << getpid() << endl;
	struct stat info;
	if (argc != 13)
	{
		cout << "Wrong arguments." << endl;
		return 1;
	}
	for (int i = 1; i <= argc - 1; i++)
	{
		if (strcmp(argv[i], "-n") == 0)
			n = atoi(argv[i + 1]);
		if (strcmp(argv[i], "-c") == 0)
			commonDirectory = argv[i + 1];
		if (strcmp(argv[i], "-i") == 0)
			inputDirectory = argv[i + 1];
		if (strcmp(argv[i], "-m") == 0)
			mirrorDirectory = argv[i + 1];
		if (strcmp(argv[i], "-b") == 0)
			bufferSize = atoi(argv[i + 1]);
		if (strcmp(argv[i], "-l") == 0)
			logfile = argv[i + 1];
	}

	unlink(logfile);
	cout << "Created process with PID " << getpid() << endl;
	struct sigaction actALRM, actINT, actQUIT, actCHLD, actUSR1, actUSR2;
	actALRM.sa_flags = 0;
	actINT.sa_flags = 0;
	actQUIT.sa_flags = 0;
	actCHLD.sa_flags = 0;
	actUSR1.sa_flags = 0;
	actUSR2.sa_flags = 0;
	actALRM.sa_handler = handlerSIGALRM;
	actINT.sa_handler = handlerSIGINT;
	actQUIT.sa_handler = handlerSIGQUIT;
	actCHLD.sa_handler = handlerSIGCHLD;
	actUSR1.sa_handler = handlerSIGUSR1;
	actUSR2.sa_handler = handlerSIGUSR2;
	sigfillset(&actALRM.sa_mask);
	sigfillset(&actINT.sa_mask);
	sigfillset(&actQUIT.sa_mask);
	sigfillset(&actCHLD.sa_mask);
	sigfillset(&actUSR1.sa_mask);
	sigfillset(&actUSR2.sa_mask);
	sigdelset(&actALRM.sa_mask, SIGCHLD);
	sigdelset(&actALRM.sa_mask, SIGUSR2);

	sigaction(SIGALRM, &actALRM, 0);
	sigaction(SIGINT, &actINT, 0);
	sigaction(SIGQUIT, &actQUIT, 0);
	sigaction(SIGCHLD, &actCHLD, 0);
	sigaction(SIGUSR1, &actUSR1, 0);
	sigaction(SIGUSR2, &actUSR2, 0);

	if (stat(mirrorDirectory, &info) != -1) //check if mirrorDirectory exists
	{
		cerr << "Mirror directory already exists." << endl;
		return 1;
	}
	mkdir(mirrorDirectory, DEFAULT_RIGHTS); //create mirrorDirectory

	if (stat(inputDirectory, &info) != 0) //check if inputDirectory does not exist
	{
		cerr << "Input directory doesn't exist." << endl;
		return 1;
	}
	mkdir(commonDirectory, DEFAULT_RIGHTS); //create commonDirectory

	//create id file inside commonDirectory
	char *idfile = new char[strlen(commonDirectory) + 10 + 3];
	sprintf(idfile, "%s/%d.id", commonDirectory, n);
	FILE *id = fopen(idfile, "r");
	if (id != NULL)          //check if id already exists
	{
		cerr << "ID file " << idfile << " already exists." << endl;
		return 1;
	}
	id = fopen(idfile, "w");
	fprintf(id, "%d\n", getpid());
	fclose(id);
	FILE *log = fopen(logfile, "a"); //open log and write id(for stat use)
	fprintf(log, "%d\n", n);
	fclose(log);
	delete[] idfile;
	idfile = NULL;

	kill(getpid(), SIGALRM); //sent to pid a SIGALRM(handler SIGALRM called)

	while (true)
		pause();

	return 0;
}

void createChilds(int j, int oneChildOnly = -1) //forks 2 childs to send and receive from other clients
{
	pid_t pids[2] = {0, 0};
	int i;

	for (i = 0; i < 2; ++i)
	{
		if ((pids[i] = fork()) < 0)
		{
			perror("fork");
			exit(1);
		}
		else if (!pids[i])
		{
			if (i == 0 && (oneChildOnly == -1 || oneChildOnly == 0)) //child 1(sender)
			{
				char *fifoname = createFifoName(n, j);	//create fifo
				mkfifo(fifoname, 0666);

				int fd = open(fifoname, O_WRONLY); 	//open fifo
				if (fd < 0)
				{
					perror("open");
					exit(1);
				}
				int error = 0;
				sendAllFiles(inputDirectory, fd, error); //send files from input

				if (error != 1) //No error occurred
				{
					unsigned short end = 0;
					write(fd, &end, 2);
				}

				close(fd);			//close and unlink fifo
				unlink(fifoname);
				delete[] fifoname;
				if (error == 1)
					exit(1);
			}
			else if (i == 1 && (oneChildOnly == -1 || oneChildOnly == 1))//child 2(receiver)
			{
				char *fifoname = createFifoName(j, n);	//create fifo
				mkfifo(fifoname, 0666);

				int fd = open(fifoname, O_RDONLY);   //open fifo
				if (fd < 0)
				{
					perror("open");
					exit(1);
				}
				int rc, error = 0;

				struct pollfd fdarray[1]; //polling begins
				fdarray[0].fd = fd;
				fdarray[0].events = POLLIN;
				rc = poll(fdarray, 1, 30 * 1000); //waits 30 seconds for a sender

				if (rc > 0 && fdarray[0].revents == POLLIN) //sender exists and has sent smth
					receiveFiles(fd, j, error); //receive files and save to mirror/cur->getId
				else if (rc == 0) //sender did not showed up in 30 sec
				{
					int parentpid = getppid();
					cout << "Poll timeout" << endl;
					kill(parentpid, SIGUSR1); //send parent SIGUSR1
				}

				if (error == 1) //Error occurred
				{
					kill(getppid(), SIGUSR2); //send parent SIGUSR2
				}

				close(fd); //close and unlink fifo
				unlink(fifoname);
				delete[] fifoname;
				if (error == 1)
					exit(1);
			}
			exit(0);
		}
		if (i == 0 && pids[i] != 0)
			known.updatePid(j, pids[i], true);
		else
			known.updatePid(j, pids[i], false);
	}
}

void handlerSIGALRM(int sig)
{
	cout << "SIGALRM received..." << endl;
	List inserted, deleted;
	scanChanges(inserted, deleted); //check commonDir for changes: new entries or absent clients, and update lists

	Listnode* cur = inserted.getStart();
	while (cur) //for every new entry, create 2 childs
	{
		known.insertNode(cur->getId()); //cur now is considered as known
		createChilds(cur->getId());
		cur = cur->getNext();
	}

	cur = deleted.getStart();
	while (cur)			//for every deleted entry
	{
		pid_t pid;
		if ((pid = fork()) < 0) //fork 1 child
		{
			perror("fork");
			exit(1);
		}
		else if (!pid)
		{
			char pathSubdirectory[strlen(mirrorDirectory) + 14];
			sprintf(pathSubdirectory, "%s/%d", mirrorDirectory, cur->getId());
			myrmDir(pathSubdirectory);//child deletes the id file of the deleted client from its mirror
			exit(255);		//SIGCHLD has been sent to parent
		}
		known.removeNode(cur->getId());
		cur = cur->getNext();
	}

	alarm(DURATION);
}

void handlerSIGINT(int sig)
{
	cout << "SIGINT received..." << endl;
	myrmDir(mirrorDirectory);  //delete mirrodDirectory
	char *filename = new char[strlen(commonDirectory) + 10 + 3];
	sprintf(filename, "%s/%d.id", commonDirectory, n); //delete id file in commonDirectory
	unlink(filename);
	delete[] filename;
	filename = NULL;
	FILE *log = fopen(logfile, "a"); //before exit, write to log file L(left) (for stats use)
	fprintf(log, "L\n");
	fclose(log);
	exit(0);
}

void handlerSIGQUIT(int sig)
{
	cout << "SIGQUIT received..." << endl;
	myrmDir(mirrorDirectory);  //delete mirrodDirectory
	char *filename = new char[strlen(commonDirectory) + 10 + 3];
	sprintf(filename, "%s/%d.id", commonDirectory, n); //delete id file in commonDirectory
	unlink(filename);
	delete[] filename;
	filename = NULL;
	FILE *log = fopen(logfile, "a"); //before exit, write to log file L(left) (for stats use)
	fprintf(log, "L\n");
	fclose(log);
	exit(0);
}

void handlerSIGCHLD(int sig)
{
	int status, pid;
	pid = waitpid(-1, &status, WNOHANG);
	if (pid != -1)
	{
		if (WEXITSTATUS(status) == 0)
		{
			cout << "File transfer from PID " << pid
					<< " completed with success" << endl;
		}
		else if (WEXITSTATUS(status) == 255)
		{
			cout << "Sub-directory has been deleted from child with PID " << pid
					<< " with success" << endl;
		}
		else
			cout << "Other exit code from pid " << pid << endl;
	}
}

void handlerSIGUSR1(int sig) //received when poll has timed out
{
	cout << "SIGUSR1 received from child process,poll timed out" << endl;
}

void handlerSIGUSR2(int sig)
{
	int pid, status;
	cout << "SIGUSR2 received from child process,error occurred" << endl;
	pid = waitpid(-1, &status, 0);
	Listnode* ln = known.searchPid(pid);
	if (ln != NULL)
	{
		if (ln->getrPid() == pid)
		{
			//receiver error
			cout << "Restarting receiver for id " << n << endl;
			if (ln->getCounterR() < 3)
			{
				//restart
				ln->incCounterR();
				createChilds(ln->getId(), 1);
			}
		}
		else
		{
			//sender error
			cout << "Restarting sender for id " << n << endl;
			if (ln->getCounterS() < 3)
			{
				//restart
				ln->incCounterS();
				createChilds(ln->getId(), 0);
			}
		}
	}
}

void scanChanges(List& inserted, List& deleted)
{
	DIR *dp;
	struct dirent * dir;
	if ((dp = opendir(commonDirectory)) == NULL)
	{
		perror("opendir");
		return;
	}
	known.resetExist();
	while ((dir = readdir(dp)) != NULL)
	{
		if (dir->d_ino == 0)
			continue;
		if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0) // Ignore . and ..
			continue;

		if (strstr(dir->d_name, ".id") != NULL) //if its an id dir
		{
			if (atoi(dir->d_name) == n)
				continue;
			if (known.searchList(atoi(dir->d_name)) == false) //new entry
			{
				inserted.insertNode(atoi(dir->d_name));
			}
			Listnode* temp = known.getStart();
			while (temp != NULL)
			{
				if (temp->getId() == atoi(dir->d_name))
					temp->setExist(true);
				temp = temp->getNext();
			}
		}
	}

	//check if there is any node in known list but not in dir
	Listnode* tmp = known.getStart();
	while (tmp != NULL)
	{
		if (tmp->getExist() == false)
			deleted.insertNode(tmp->getId());
		tmp = tmp->getNext();
	}

	closedir(dp);
}

void myrmDir(char* name)
{
	DIR *dp;
	struct dirent *dir;
	char *newname;

	if ((dp = opendir(name)) == NULL) //Open directory
	{
		perror("opendir");
		return;
	}

	while ((dir = readdir(dp)) != NULL) // Read contents till end
	{
		if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
			continue; // Ignore . and ..
		if (dir->d_ino == 0) // Ignore removed entries
			continue;
		newname = new char[strlen(name) + strlen(dir->d_name) + 2];
		strcpy(newname, name);
		strcat(newname, "/");
		strcat(newname, dir->d_name);
		struct stat sbuf;
		stat(newname, &sbuf);

		if (S_ISDIR(sbuf.st_mode)) //newname is a directory
		{
			myrmDir(newname);
		}
		else if (S_ISREG(sbuf.st_mode)) // newname is a file
		{
			unlink(newname);
		}

		delete[] newname;
	}
	closedir(dp);
	struct stat sbuf;
	stat(name, &sbuf);
	rmdir(name);
}

char* createFifoName(int from, int to)
{
	//fifo name example: common/1_to_2.fifo
	char* str = new char[strlen(commonDirectory) + 20];
	sprintf(str, "%s/%d_to_%d.fifo", commonDirectory, from, to);
	return str;
}

void sendAllFiles(char* directory, int fd, int &error)
{
	DIR *dp;
	struct dirent *dir;
	char *newname;

	if ((dp = opendir(directory)) == NULL) // Open directory
	{
		perror("opendir");
		return;
	}

	while ((dir = readdir(dp)) != NULL) //Read contents till end
	{
		if (strcmp(dir->d_name, ".") == 0 || strcmp(dir->d_name, "..") == 0)
			continue; 										//Ignore . and ..
		if (dir->d_ino == 0) 							//Ignore removed entries
			continue;
		newname = new char[strlen(directory) + strlen(dir->d_name) + 2];
		strcpy(newname, directory);
		strcat(newname, "/");
		strcat(newname, dir->d_name);
		struct stat sbuf;
		stat(newname, &sbuf);

		if (S_ISDIR(sbuf.st_mode)) //newname is a directory
		{
			sendAllFiles(newname, fd, error);
		}
		else if (S_ISREG(sbuf.st_mode)) // newname is a file
		{
			sendFile(newname, fd, error);
		}
		delete[] newname;
	}
	closedir(dp);
}

void sendFile(char* filename, int fd, int& error)
{
	//input_1/one/two/three.txt
	char* tmp = strchr(filename, '/');
	char* shortFilename = tmp + 1;  //skip the name of input dir
	unsigned short int fileLen = strlen(shortFilename); //2 bytes length of name of file
	if (write(fd, &fileLen, 2) < 0)
	{
		error = 1;
		return;
	}
	if (write(fd, shortFilename, fileLen + 1) < 0)
	{
		error = 1;
		return;
	}

	struct stat buf;
	stat(filename, &buf);
	int size = buf.st_size;
	if (write(fd, &size, 4) < 0) //4 bytes length of file
	{
		error = 1;
		return;
	}

	int fileFd = open(filename, O_RDONLY);
	int bytesRemaining = size, bytes;
	while (bytesRemaining > 0)
	{
		bytes = (bytesRemaining < bufferSize) ? bytesRemaining : bufferSize;
		char buffer[bytes];
		memset(buffer, 0, bytes);
		read(fileFd, buffer, bytes); //read from file bytes size
		if (writeAll(bytes, fd, buffer) < 0) //write them to the pipe
		{
			error = 1;
			close(fileFd);
			return;
		}
		bytesRemaining -= bytes;
	}
	close(fileFd);
	cout << "Sent file " << filename << ", size: " << size << endl;
	FILE *log = fopen(logfile, "a"); //write to log name and size of the sent file
	fprintf(log, "S %s %d\n", filename, size);
	fclose(log);
}

int receiveFile(int fd, int j, int& error)
{
	unsigned short int fileLen;
	int size;
	FILE *log;
	if (read(fd, &fileLen, 2) <= 0)
	{
		error = 1;
		return -1;
	}
	if (fileLen == 0)
		return -1;
	char shortFilename[fileLen];
	memset(shortFilename, 0, fileLen);
	if (read(fd, shortFilename, fileLen + 1) <= 0) //first read the name
	{
		error = 1;
		return -1;
	}
	if (read(fd, &size, 4) <= 0)  //read the size
	{
		error = 1;
		return -1;
	}

	char filename[strlen(shortFilename) + strlen(mirrorDirectory) + 10];
	sprintf(filename, "%s/%d/%s", mirrorDirectory, j, shortFilename);

	createSubFolders(filename);	//create subfolders

	int fileFd = creat(filename, 0644); //create he filename in the specific path

	int bytesRemaining = size, bytes;
	while (bytesRemaining > 0)
	{
		bytes = (bytesRemaining < bufferSize) ? bytesRemaining : bufferSize;
		char buffer[bytes];
		memset(buffer, 0, bytes);
		if (readAll(bytes, fd, buffer) < 0) //read bytes from buffer
		{
			error = 1;
			close(fileFd);
			return -1;
		}
		write(fileFd, buffer, bytes); //write them to the new file
		bytesRemaining -= bytes;
	}

	close(fileFd);
	cout << "Received file " << filename << ", size: " << size << endl;

	log = fopen(logfile, "a");//write to log name and size of the received file
	fprintf(log, "R %s %d\n", filename, size);
	fclose(log);
	return 0;
}

void receiveFiles(int fd, int j, int& error)
{
	int a = 0;
	while (a != -1)
		a = receiveFile(fd, j, error);
}

void createSubFolders(char* path)
{
	//path example: mirror_1/2/one/two/three.txt
	char term = '\0', slash = '/';
	char* end = strchr(path, '/');
	while (end != NULL)
	{
		*end = term;
		mkdir(path, 0755);
		*end = slash;
		end = strchr(++end, '/');
	}
}

int writeAll(int size, int fd, char* buffer)
{
	int sent, b = 0;
	for (sent = 0; sent < size; sent += b)
	{
		b = write(fd, buffer + sent, size - sent);
		if (b < 0)
		{
			return -1;
		}
	}
	return 0;
}

int readAll(int size, int fd, char* buffer)
{
	int received, b = 0;
	for (received = 0; received < size; received += b)
	{
		b = read(fd, buffer + received, size - received);
		if (b <= 0)
		{
			return -1;
		}
	}
	return 0;
}
