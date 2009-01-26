/* dm.c - deathmatch
 * PUBLIC DOMAIN - Jon Mayo - January 25, 2009
 * a tiny unmaintanable telnet game
 */
#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define S(x) sizeof(x)
#define N(x) (S(x)/S(*x))
#define CK(s) if((s)<0) { perror(#s); exit(1); }
#define F(x) for(a=0;a<N(c);a++) { x; }
#define CM(f) void f(int b, char *s)
#define CH(f) void f(int b)
#define R "\r\n"
#define KS ss(b, B.s);
#define B c[b]
#define A c[a]
enum {
	no, so, we, ea
};
const char *q[]={"quit", "look", "n", "north", "s", "south", "w", "west", "e", "east", "say", "help", "name"};
struct {
	int f, r, s;
	char n[16], b[999];
} c[9];
struct {
	int p[4];
	char *d;
} r[] = {
	{{-1,1,-1,-1}, "A large grassy field."R"To the south is a gate"R},
	{{0,2,-1,-1}, "A large iron gate."R"To the north is a field, to the south is an ampitheater."R},
	{{1,-1,-1,-1}, "An abandoned ampitheater."R"To the north is a gate."R},
};
void wr(int b, const char *s) {
	CK(write(B.f, s, strlen(s)));
}
CH(cl) {
	int a;
	close(B.f);
	B.f=0;
	F(if(A.f&&A.r==B.r) { wr(a, B.n); wr(a, " has disconnected"R); });
}
void ss(int b, int s) {
	B.s=s;
	if(s==0) {
		wr(b, ">");
	}
}
CM(c_q) {
	cl(b);
}
CM(c_l) {
	int a, f;
	wr(b, r[B.r].d);
	f=0;
	F(if(a!=b&&A.f&&A.r==B.r) { wr(b, A.n); wr(b, " "); f++; });
	if(f==1) {
		wr(b, "is here"R);
	} else if(f) {
		wr(b, "are here"R);
	}
	KS;
}
void go(int b, int d) {
	int a, t=r[B.r].p[d];
	if(t<0) {
		wr(b,"You can't go that way"R);
	} else {
		F(if(a!=b&&A.f&&A.r==B.r) { wr(a, B.n); wr(a, " leaves."R); });
		B.r=t;
		F(if(a!=b&&A.f&&A.r==B.r) { wr(a, B.n); wr(a, " enters."R); });
	}
	c_l(b, "");
}
CM(c_n) {
	go(b, no);
}
CM(c_s) {
	go(b, so);
}
CM(c_w) {
	go(b, we);
}
CM(c_e) {
	go(b, ea);
}
CM(c_y) {
	int a;
	wr(b, "You say \"");
	wr(b, s);
	wr(b, "\""R);
	F(if(a!=b&&A.f&&A.r==B.r) { wr(a, B.n); wr(a, " says \""); wr(a, s); wr(a, "\""R); });
	KS;
}
CM(c_h) {
	int j;
	for(j=0;j<N(q);j++) {
		wr(b, q[j]);
		wr(b, " ");
	}
	wr(b, R);
	KS;
}
CM(c_a) {
	int a;
	char *p;
	if(strlen(s)>=S(c->n)) s[S(c->n)-1]=0;
	for(p=s;*p;p++) if(!isalnum(*p)) *p='_';
	if(*s) {
		F(if(a!=b&&A.f&&A.r==B.r&&!strncmp(A.n, s, S(c->n))) { wr(b, "Name already used"R); KS; return; } );

		F(if(a!=b&&A.f&&A.r==B.r) { wr(a, B.n); wr(a, " is now known as "); wr(a, s); wr(a, R); });
		strcpy(B.n, s);
		wr(b, "Your name is now \"");
		wr(b, B.n);
		wr(b, "\""R);
	}
	KS;
}
CM((*f[N(q)]))={c_q, c_l, c_n, c_n, c_s, c_s, c_w, c_w, c_e, c_e, c_y, c_h, c_a};
CH(sl) {
	int j, l;
	if(B.b[0]) {
		for(j=0;j<N(q);j++) {
			l=strcspn(B.b," ");
			if(l==strlen(q[j]) && !strncmp(B.b, q[j], l)) {
				while(isspace(B.b[l])) l++;
				f[j](b, B.b+l);
				goto x;
			}
		}
		wr(b, "Try 'help' for commands."R);
	}
	KS;
x:
	B.b[0]=0;
}
CH(nc) {
	B.b[0]=0;
	B.r=0;
	B.s=0;
	snprintf(B.n, S(B.n), "Someone%u", b+1);
	c_l(b,"");
}
int main(void) {
	int s, a, n, m, t;
	struct sockaddr_in x;
	fd_set z;
	socklen_t l;
	char h[2]={0};
	signal(SIGPIPE, SIG_IGN);
	CK(s=socket(PF_INET, SOCK_STREAM, 0));
	memset(&x, 0, S(x));
	x.sin_port=htons(5555);
	CK(bind(s, &x, S(x)));
	CK(listen(s,1));
	for(;;) {
		FD_ZERO(&z);
		FD_SET(s, &z);
		m=s;
		F(if(A.f) { FD_SET(A.f, &z); if(A.f>m) m=A.f; });
		CK(n=select(m+1, &z, 0, 0, 0));
		if(FD_ISSET(s, &z)) {
			l=S(x);
			CK(t=accept(s, &x, &l));
			F(if(!A.f) { A.f=t; nc(a); break; });
			if(a==N(c)) {
				close(t);
			}
		}
		F(if(A.f&&FD_ISSET(A.f, &z)) { CK(t=read(A.f, h, 1)); if(t==0) { cl(a); } else if(*h=='\r') sl(a); else if(*h!='\n') strcat(A.b, h); } );
	}
	return 0;
}
