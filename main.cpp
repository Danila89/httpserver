#include <cstdio>
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <boost/bind.hpp>
#include <boost/asio.hpp>
#include <boost/thread.hpp>
#include <string>
#include <cstring>
#include <sys/stat.h>
using boost::asio::ip::tcp;

std::ofstream lg();
boost::asio::io_service io_serv;
void iosrun()
{
	io_serv.run();
}
class session
{
private:
	boost::asio::streambuf buf;
	tcp::socket sock;
public:
	session(boost::asio::io_service &io): sock(io) {}
	tcp::socket &socket() {return sock;}
	void start(){
		async_read_until(sock,buf,'\n',boost::bind(&session::handle_read,this,
                                boost::asio::placeholders::error));
	}
	void handle_read(const boost::system::error_code &error){
		if(!error){
			std::istream ii(&buf);
			std::ostream oo(&buf);
			std::string cmd, fl;
			ii>>cmd;
			char t;
			while(ii.get(t){
				if(t!='?') fl.push_back(t);
				else break;
			}
			lg<<"Received "<<cmd<<" command for"<<fl<<std::endl;
			if(cmd!="GET") delete this;
			std::ifstream fstr(fl);
			if(fstr.fail()) {
				oo<<"HTTP/1.0 404 Not Found"<<'\r'<<'\n';
				oo<<"Content-length: 0"<<'\r'<<'\n';
				oo<<"Content-Type: text/html"<<'\r'<<'\n'<<'\r'<<'\n';
			}
			else{
				struct stat st;
				stat(fl.c_str(),&st);
				int sz=st.st_size;
				char c;
				oo<<"HTTP/1.0 200 OK"<<'\r'<<'\n';
				oo<<"Content-length: "<<sz<<'\r'<<'\n';
				oo<<"Content-Type: text/html"<<'\r'<<'\n'<<'\r'<<'\n';
				while(fstr.get(c)) oo.put(c);
			}
			fstr.close();
			async_write(sock,buf,boost::bind(&session::handle_write,
				this,boost::asio::placeholders::error));
		}
		else delete this;
	}
	void handle_write(const boost::system::error_code &error)
	{
//		sock.shutdown();
		sock.close();
		delete this;
	}
};

				
class server
{
private:
	boost::asio::io_service &ios;
	tcp::acceptor acc;
public:
	server(boost::asio::io_service &io, const char* ip, short port): ios(io),
		acc(ios,tcp::endpoint(boost::asio::ip::address::from_string(ip),port))
	{
		session* new_s = new session(ios);
		acc.async_accept(new_s->socket(),
			boost::bind(&server::handle_accept,this,new_s,
					boost::asio::placeholders::error));
	}
	void handle_accept(session* new_s, const boost::system::error_code &error)
	{
		if(!error){
			new_s->start();
			new_s=new session(ios);
		acc.async_accept(new_s->socket(),
			boost::bind(&server::handle_accept,this,new_s,
					boost::asio::placeholders::error));
		}
		else delete new_s;
	}
}; 

int main(int argc, char* argv[])
{
	char* opts="h:p:d:";
	char ip[20];
	short port;
	char dir[50];
	char res;
	while((res=getopt(argc,argv,opts))!=-1){
		switch(res){
		case 'h': strcpy(ip,optarg); break;
		case 'p': port=atoi(optarg); break;
		case 'd': strcpy(dir,optarg); break;
		default: std::cerr<<"Unexpected parameter"<<'\n';
		}
	}
	int p=fork();
	if(p==-1) perror("Fork");
	if(p) return 0;
	else{
		if(setsid()==-1) perror("Setsid");
		if(chroot(dir)==-1) perror("Chroot");
		if(chdir("/")==-1) perror("Chdir");
		lg.open("server.log");
		fclose(stdin);
		fclose(stdout);
		fclose(stderr);
		server s(io_serv,ip,port);
		for(int i=0; i<3; i++){
			boost::thread(iosrun);
		}
		iosrun();
	return 0;
	}
}
