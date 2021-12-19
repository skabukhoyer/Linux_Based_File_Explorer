#include <termios.h>
#include <unistd.h>
#include<stdlib.h>
#include<stdio.h>
#include<ctype.h>
#include<errno.h>
#include<sys/types.h>
#include<sys/wait.h>
#include<sys/stat.h>
#include<dirent.h>
#include<sys/stat.h>
#include<unistd.h>
#include<string.h>
#include<pwd.h>
#include<grp.h>
#include<time.h>
#include<fcntl.h>
#include<bits/stdc++.h>
#include<sys/types.h>
#include<stdlib.h>
#include<sys/ioctl.h>

using namespace std;

string root;
stack<string> fd;
stack<string> bk;


struct termios orig_termios;
struct editorConfig {
  int cx, cy;
  int rowoff;
  int screenrows;
  int screencols;
  int numrows;
};
struct editorConfig E;
void die(const char *s){
	perror(s);
	exit(1);
}
int getCursorPosition(int *rows, int *cols) {
  char buf[32];
  unsigned int i = 0;
  if (write(STDOUT_FILENO, "\x1b[6n", 4) != 4) return -1;
  while (i < sizeof(buf) - 1) {
    if (read(STDIN_FILENO, &buf[i], 1) != 1) break;
    if (buf[i] == 'R') break;
    i++;
  }
  buf[i] = '\0';
  if (buf[0] != '\x1b' || buf[1] != '[') return -1;
  if (sscanf(&buf[2], "%d;%d", rows, cols) != 2) return -1;
  return 0;
}
int getWindowSize(int *rows, int *cols) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1 || ws.ws_col == 0) {
    if (write(STDOUT_FILENO, "\x1b[999C\x1b[999B", 12) != 12) return -1;
    return getCursorPosition(rows, cols);
  } else {
    *cols = ws.ws_col;
    *rows = ws.ws_row;
    return 0;
  }
}
void initEditor() {
  E.cx = 0;
  E.cy = 0;
  E.rowoff = 0;
  E.numrows = 0;
  //E.row = NULL;
  if (getWindowSize(&E.screenrows, &E.screencols) == -1) die("getWindowSize");
}
void disableRawMode() {
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
  printf("\033[H\033[J");
}
void enableRawMode() {
  tcgetattr(STDIN_FILENO, &orig_termios);
  atexit(disableRawMode);
  struct termios raw = orig_termios;
  raw.c_iflag &= ~(ICRNL | IXON);
  //raw.c_oflag &= ~(OPOST);
  raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);
  tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}
void read_dir(vector<string> &v){
	char cwd[PATH_MAX];
	getcwd(cwd, sizeof(cwd));
	DIR * dir = opendir(".");
	struct dirent * sd;
	v.push_back(".");
	if(cwd!=root){ v.push_back(".."); }
	while((sd=readdir(dir))!=NULL){
		if(sd->d_name[0]!='.'){ v.push_back(sd->d_name) ;}
	}
}
void show(vector<string> &v,int x, int y){

	struct stat fileStat;
	char name[100], mtime[100];
	for(int i=x;i<=y;i++){		
		strcpy(name, v[i].c_str());
		stat(v[i].c_str(), &fileStat);
	   
			    if(strlen(name)>20){
			    	name[20]='.';
			    	name[21]='.';
			    	name[22]='\0';
			    	}
			    cout<<setw(30)<<left<<name<<setw(15)<<left;
			    printf( (S_ISDIR(fileStat.st_mode)) ? "d" : "-");
			    printf( (fileStat.st_mode & S_IRUSR) ? "r" : "-");
			    printf( (fileStat.st_mode & S_IWUSR) ? "w" : "-");
			    printf( (fileStat.st_mode & S_IXUSR) ? "x" : "-");
			    printf( (fileStat.st_mode & S_IRGRP) ? "r" : "-");
			    printf( (fileStat.st_mode & S_IWGRP) ? "w" : "-");
			    printf( (fileStat.st_mode & S_IXGRP) ? "x" : "-");
			    printf( (fileStat.st_mode & S_IROTH) ? "r" : "-");
			    printf( (fileStat.st_mode & S_IWOTH) ? "w" : "-");
			    printf( (fileStat.st_mode & S_IXOTH) ? "x" : "-");
			    
			    cout<<setw(15)<<right<<fileStat.st_size<<setw(10)<<left<<" Bytes";
				struct passwd *pw = getpwuid(fileStat.st_uid);
				struct group  *gr = getgrgid(fileStat.st_gid);
				if(pw!=0)
				cout<<setw(15)<<left<<pw->pw_name;
				if(gr!=0)
				cout<<setw(15)<<left<<gr->gr_name;
				
			    strcpy(mtime, ctime(&fileStat.st_mtime));
			    cout<<"\t"<<mtime;
		}
}
void gotoxy(int x,int y)    
{
    printf("%c[%d;%df",0x1B,x,y);
}
 void pushStack(stack<string> &stk, char const *str){
	if(stk.empty()){ stk.push(str); return;}
	if(stk.top()!=str){ stk.push(str); }
}	    	
char normal(){
	printf("\033[H\033[J");
	initEditor();
	vector<string> content;
	read_dir(content);
	int dirSize=content.size();
	E.numrows=dirSize;
	E.cx=0;
	E.rowoff=ceill(120.0/E.screencols);
	E.screenrows=E.screenrows-1;
	E.cy=min(dirSize/E.rowoff, E.screenrows/E.rowoff)-1;
	show(content,E.cx,E.cy); 
	int cursor=0;
	gotoxy(1,1);
	char ch;
	while(1){
		ch= cin.get();
		if(ch=='q'){ return ch; }
		else if(ch==':'){ return ch;}
		else if(13 == int(ch)){
			struct stat stats;
			stat(content[cursor].c_str(),&stats);
			bool isDir= (stats.st_mode & S_IFDIR)? 1: 0;
			if(isDir){
				if(content[cursor]=="."){ continue; }
				string currD=content[cursor];
				char cwd[PATH_MAX];
				getcwd(cwd,PATH_MAX);
				currD.insert(0,"/");
				currD= cwd + currD;
				if(content[cursor]==".."){ pushStack(fd,cwd); }
				else { pushStack(bk, cwd); }
				int err = chdir(currD.c_str());
				if(err == -1){ cout<<"ERROR!"<<endl; }
				content.clear();
				printf("\033[H\033[J");
				initEditor();
				read_dir(content);
				dirSize=content.size();
				E.numrows=dirSize;
				E.cx=0;
				E.rowoff=ceill(120.0/E.screencols);
				E.screenrows=E.screenrows-1;
				E.cy=min(dirSize/E.rowoff, E.screenrows/E.rowoff)-1;
				show(content,E.cx,E.cy); 
				cursor=0;
				gotoxy(1,1);
			}
			else {
				if(fork()==0){
					string str=content[cursor];
					char *args[3];
					string command= "vi";
					args[0] = (char *)command.c_str();
					args[1] = (char *)str.c_str();
					args[2] = NULL;
					printf("\033[H\033[J");
					if(execvp(args[0],args) == -1){
						perror("exec");
					}
				}
				else{
					wait(0);
				}
				return 'x' ;
			}
			//gotoxy(50,20);
		}
		else if(104 == int(ch)){
			
			printf("\033[H\033[J");
			while(!fd.empty()){ fd.pop();}
			int err=chdir(root.c_str());
			if(err==-1){cout<<"ERROR"<<endl;}
			content.clear();
				printf("\033[H\033[J");
				initEditor();
				read_dir(content);
				dirSize=content.size();
				E.numrows=dirSize;
				E.cx=0;
				E.rowoff=ceill(120.0/E.screencols);
				E.screenrows=E.screenrows-1;
				E.cy=min(dirSize/E.rowoff, E.screenrows/E.rowoff)-1;
				show(content,E.cx,E.cy); 
				cursor=0;
				gotoxy(1,1);
		}
		else if( ch==127){
			
			char cwd[PATH_MAX];
			getcwd(cwd,PATH_MAX);
			if(cwd!=root){
				string currD;
				currD=cwd;
				pushStack(fd,cwd);
				while(currD[currD.size()-1]!='/'){ currD.pop_back() ;}
				currD.pop_back();
				int err=chdir(currD.c_str());
				if(err==-1){ cout<<"ERROR"<<endl;}
				content.clear();
				printf("\033[H\033[J");
				initEditor();
				read_dir(content);
				dirSize=content.size();
				E.numrows=dirSize;
				E.cx=0;
				E.rowoff=ceill(120.0/E.screencols);
				E.screenrows=E.screenrows-1;
				E.cy=min(dirSize/E.rowoff, E.screenrows/E.rowoff)-1;
				show(content,E.cx,E.cy); 
				cursor=0;
				gotoxy(1,1);
			}
			
		}
		else if(ch=='C'){
			if(!fd.empty()){
				char cwd[PATH_MAX];
				getcwd(cwd,PATH_MAX);
				pushStack(bk,cwd);
				string currD=fd.top();
				fd.pop();
				int err=chdir(currD.c_str());
				if(err==-1){ cout<<"ERROR"<<endl;}
				content.clear();
				printf("\033[H\033[J");
				initEditor();
				read_dir(content);
				dirSize=content.size();
				E.numrows=dirSize;
				E.cx=0;
				E.rowoff=ceill(120.0/E.screencols);
				E.screenrows=E.screenrows-1;
				E.cy=min(dirSize/E.rowoff, E.screenrows/E.rowoff)-1;
				show(content,E.cx,E.cy); 
				cursor=0;
				gotoxy(1,1);
			}
		}
		else if(ch=='D'){
			if(!bk.empty()){
				char cwd[PATH_MAX];
				getcwd(cwd,PATH_MAX);
				pushStack(fd,cwd);
				string currD=bk.top();
				bk.pop();
				int err=chdir(currD.c_str());
				if(err==-1){ cout<<"ERROR"<<endl;}
				content.clear();
				printf("\033[H\033[J");
				initEditor();
				read_dir(content);
				dirSize=content.size();
				E.numrows=dirSize;
				E.cx=0;
				E.rowoff=ceill(120.0/E.screencols);
				E.screenrows=E.screenrows-1;
				E.cy=min(dirSize/E.rowoff, E.screenrows/E.rowoff)-1;
				show(content,E.cx,E.cy); 
				cursor=0;
				gotoxy(1,1);
			}
		}
		else if(ch=='A'){
			if(cursor> E.cx && E.cy>= cursor){
				cursor--;
				printf("\033[%dA",E.rowoff);
			}
		}
		else if(ch=='B'){
			if(cursor >= E.cx && cursor < E.cy){
				cursor++;
				printf("\033[%dB",E.rowoff);
			}
		}
		else if(ch=='k'){
			if(dirSize > (E.cy-E.cx+1) && E.cx -1 >=0){ 
				show(content,--(E.cx),--(E.cy)); 
				cursor=E.cx; 
			}
			gotoxy(1,1);
				
		}
		else if(ch=='l'){
			if(dirSize > (E.cy-E.cx+1) && E.cy +1 < dirSize){ 
				show(content,++(E.cx),++(E.cy)); 
				cursor=E.cy; 
			}
			gotoxy((E.cy - E.cx +1)*E.rowoff, 1);
		}
		else{continue;}
		
	}
}
void available_commands(){
	gotoxy(100,1);
	cout<<"\033[91mCommands Available (all path should be absolute from root i,e., where application was started :)\033[1m";
	cout<<"\n\x1B[32m";
	cout<<"COPY:- copy <src_file/dir> <dest_dir>\n";
	cout<<"MOVE:- move <src_file/dir> <dest_dir>\n";
	cout<<"RENAME:- rename <old_filename> <new_filename>\n";
	cout<<"CREATE:- create_file <filename> <dest_path>\n";
	cout<<"CREATE:- create_dir <dir_name> <dest_path>\n";
	cout<<"DELETE:- delete_file <file_path>\n";
	cout<<"DELETE:- delete_dir <file_path>\n";
	cout<<"GOTO:- goto <location>\n";
	cout<<"SEARCH:- search <file_name>\n";
	cout<<"SEARCH:- search <dir_name>\n";
	cout<<"\033[32m";
	cout<<"Press ESC to return to normal mode and q for close application";
	gotoxy(0,0);
	cout<<"\x1B[0m";
	return;
}
void remove_dir( const char *path)
{
        struct dirent *entry = NULL;
        DIR *dir = NULL;
        dir = opendir(path);
        while(entry = readdir(dir))
        {   
                DIR *sub_dir = NULL;
                FILE *file = NULL;
                char* abs_path = new char[256];
                 if ((*(entry->d_name) != '.') || ((strlen(entry->d_name) > 1) && (entry->d_name[1] != '.')))
                {   
                        sprintf(abs_path, "%s/%s", path, entry->d_name);
                        if(sub_dir = opendir(abs_path))
                        {   
                                closedir(sub_dir);
                                remove_dir(abs_path);
                        }   
                        else 
                        {   
                                if(file = fopen(abs_path, "r"))
                                {   
                                        fclose(file);
                                        remove(abs_path);
                                }   
                        }   
                }
                delete[] abs_path;   
        }   
        remove(path);
}
bool search(string path, string fd){
	DIR *dir;
	struct dirent *ent;
	if ((dir = opendir (path.c_str())) != NULL) {	 
	string s="";
	  while ((ent = readdir (dir)) != NULL) {
	  	s=ent->d_name;
	  	if(s[0]!='.'){
		    	if(s == fd){ return true; }
		    	struct stat stats;
			stat(s.c_str(),&stats);				
			bool isDir= (stats.st_mode & S_IFDIR)? 1: 0;
			if(isDir==1 && search(path+"/"+s, fd)) { return true; } 	  
		}  
	  }
	  closedir (dir);
	  return false;
	} 
	else {	 
	  perror ("opendir");
	  //return EXIT_FAILURE;
	}
	return false;
}
void copyFile(char const *sorc, char const *des){
	std::ifstream src(sorc, std::ios::binary);
	std::ofstream dest(des, std::ios::binary);
	dest << src.rdbuf();
	src.close();
	dest.close();
					
}
void copy_directory(string src_dir,string dest_dir){
	int status=mkdir(dest_dir.c_str(),0777);
	if(status == -1){ cout<<"Error in directory creation: "<<dest_dir<<endl; }
	DIR *dir;
	    struct dirent *ent;
	    if ((dir = opendir (src_dir.c_str())) != NULL) {

		while ((ent = readdir (dir)) != NULL) { 
		    if (ent->d_name != std::string(".")){         
		        if (ent->d_name != std::string("..")){     

		            std::string copy_sorc = src_dir + "/" + ent->d_name;
		            std::string copy_dest = dest_dir + "/" + ent->d_name;
		 		struct stat stats;
				stat(ent->d_name,&stats);				
				bool isDir= (stats.st_mode & S_IFDIR)? 1: 0;
				if(isDir== 1) { copy_directory(copy_sorc,copy_dest) ;}
		            	else{copyFile(copy_sorc.c_str(), copy_dest.c_str()); }
		        }
		    }
		}
		closedir (dir);
	    }
	    else{
		perror ("opendir");
		//return EXIT_FAILURE;
	    }
}
void move_directory( string src_dir, string dest_dir){
	copy_directory(src_dir, dest_dir);
	remove_dir(src_dir.c_str());
	
}
char canonical(){
	printf("\033[H\033[J");
	available_commands();
	char c;
	vector<string> args;
	vector<char> str;
	cout<<"# ";
	while((c=cin.get())!=27){
		if(c=='q'){ return c; }
		else if(c==127){
			if(!str.empty()){
				str.pop_back();
				cout<<"\b ";
				printf("\033[%dD",1);
			}
		}
		else if(c==13){
			cout<<"\n";
			string temp="";
			for(int j=0;j<str.size();j++){
				if(str[j]==' '){ args.push_back(temp); temp=""; }
				else{ temp +=str[j];}
			}
			if(temp!=""){ args.push_back(temp);}
			str.clear();
			
			int n=args.size();
			if(args[0]=="copy"){
				if(n<3){ cout<<"INVALID CMD!"<<endl; args.clear(); cout<<"# "; continue;}
				string DEST=args[n-1],SRC="";
				
				if(DEST[0]=='~'){ DEST.erase(0,1) ;}
				DEST=root+DEST+"/";
				string abc="";
				char cwd[PATH_MAX];
				getcwd(cwd,PATH_MAX);
				for(int i=1;i<n-1;i++){
					SRC=args[i];
					struct stat stats;
					stat(SRC.c_str(),&stats);				
					bool isDir= (stats.st_mode & S_IFDIR)? 1: 0;
					abc=DEST+args[i];
					if(isDir){
						SRC=cwd;
						SRC +="/" + args[i];
						copy_directory(SRC,abc);
					
					}
					else{
						std::ifstream src(SRC, std::ios::binary);
						std::ofstream dest(abc, std::ios::binary);
						dest << src.rdbuf();
						src.close();
						dest.close();
					}
				}
				cout<<"copy done "<<endl;
				
			}
			else if(args[0]=="move"){
				if(n<3){ cout<<"INVALID CMD!"<<endl; args.clear(); cout<<"# "; continue;}
				string DEST=args[n-1],SRC="";
				char cwd[PATH_MAX];
				getcwd(cwd,PATH_MAX);
				string del=cwd;
				if(DEST[0]=='~'){ DEST.erase(0,1) ;}
				DEST=root+DEST+"/";
				string abc="";
				for(int i=1;i<n-1;i++){
					SRC=args[i];
					abc=DEST+args[i];
					struct stat stats;
					stat(SRC.c_str(),&stats);				
					bool isDir= (stats.st_mode & S_IFDIR)? 1: 0;
					if(isDir == 1) {
						SRC=cwd;
						SRC+="/"+args[i];
						move_directory(SRC,abc); 
					}
					else{
						std::ifstream src(SRC, std::ios::binary);
						std::ofstream dest(abc, std::ios::binary);
						dest << src.rdbuf();
						del +="/"+args[i];
						unlink(del.c_str());
					}
				
				}
				cout<<"move done"<<endl;
			}
			else if(args[0]=="rename"){
				if(n!=3) {cout<<"INVALID CMD! "<<endl; args.clear(); cout<<"# "; continue;}
				string newFile=args[2];
				string oldFile=args[1];
				if(rename(oldFile.c_str(), newFile.c_str()) !=0)
				{ cout<<"ERROR in renameing"<<endl; args.clear(); cout<<"# "; continue;}
				else{ cout<<"rename successfull"<<endl;}
			}
			else if(args[0]=="create_file"){
				if(n!=3){ cout<<"INVALID CMD! "<<endl; args.clear(); cout<<"# "; continue;}
				string file_path="", dest_dir=args[2];
				if(dest_dir[0]=='.'){
					char cwd[PATH_MAX];
					getcwd(cwd,PATH_MAX);
					file_path=cwd ;
					file_path += "/"+args[1];
				}
				else if(dest_dir[0]=='~'){ dest_dir.erase(0,1) ;
					file_path =root+dest_dir+"/"+args[1];
				}
				else { file_path =root+dest_dir+"/"+args[1];}
				
				mode_t mode = S_IRUSR | S_IWUSR|S_IXUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
				int err=open(file_path.c_str(),O_CREAT|O_WRONLY|O_TRUNC,mode);
				if(err==-1){ cout<<"Error in creating file "<<endl;}
				else{ cout<<"successfull"<<endl;}				
				
			}
			else if(args[0]=="create_dir"){
				if(n!=3){ cout<<"INVALID CMD! "<<endl; args.clear(); cout<<"# "; continue;}
				string file_path="", dest_dir=args[2];
				if(dest_dir[0]=='.'){
					char cwd[PATH_MAX];
					getcwd(cwd,PATH_MAX);
					file_path=cwd;
					file_path += "/"+args[1];
				}
				else if(dest_dir[0]=='~'){ dest_dir.erase(0,1) ;
					file_path =root+dest_dir+"/"+args[1];
				}
				else{ file_path =root+dest_dir+"/"+args[1]; }
				int status=mkdir(file_path.c_str(),0777);
				if(status == -1){ cout<<"Error in creation"<<endl; }
				else{ cout<<"directory is created in: "<<file_path<<endl;}
			}
			else if(args[0]=="delete_file"){
				if(n!=2){ cout<<"INVALID CMD"<<endl; args.clear(); cout<<"# "; continue;}
				string del_path=args[1];
				if(del_path[0]=='~'){ del_path.erase(0,1) ;}
				del_path = root + del_path ;
				int err=unlink(del_path.c_str());
				if(err==0){ cout<<"successfull"<<endl;}
				else{ cout<<"error"<<endl;}
				
			}
			else if(args[0]=="delete_dir"){
				if(n!=2) { cout<<"INVALID CMD! "<<endl;args.clear(); cout<<"# "; continue; }
				string dir_path=args[1];
				if(dir_path[0]=='~'){ dir_path.erase(0,1); }
				dir_path= root + dir_path;
				remove_dir(dir_path.c_str());
				cout<<"Successfull"<<endl;
			}
			else if(args[0]=="search"){
				if(n!=2) { cout<<"INVALID CMD! "<<endl; args.clear(); cout<<"# "; continue; }
				char cwd[PATH_MAX];
				getcwd(cwd,PATH_MAX);
				string path=cwd;
				if(search(path,args[1])){ cout<<"true"<<endl;}
				else{ cout<<"false"<<endl;}
			}
			else if(args[0]=="goto"){
				if(n!=2) { cout<<"INVALID CMD! "<<endl; args.clear(); cout<<"# "; continue; }
				string sDirectory=args[1];
				if(sDirectory[0]=='~'){ sDirectory.erase(0,1);}
				sDirectory=root+sDirectory;
				if (chdir(sDirectory.c_str()) == -1) {
   					cout<<"Error in chdir "<<endl;
				}
				else{ cout<<"Sucessfull"<<endl; }
			}
			
			else { cout<<"INVALID CMD"<<endl ;}
			args.clear(); cout<<"# ";
				
		}
		else{
			
			str.push_back(c);
			cout<<c;
		}
	}
	return 'x';
}
int main(){
	printf("\033[H\033[J");
	enableRawMode();
	//initEditor();
	char cwd[PATH_MAX];
	getcwd(cwd, sizeof(cwd));
	root=cwd;
	char ch;
	while(1)
	{
		ch=normal();
		if(ch=='q'){ break; }
		else if(ch==':') { 
			ch=canonical(); 
			printf("\033[H\033[J");
			if(ch=='q'){ break; }
		}
		else{ continue; }
	}
		
	return 0;
}
	
