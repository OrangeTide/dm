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
#define NR(x) (S(x)/S(*x))
#define CK(s) if((s)<0) { perror(#s); exit(1); }
#define F(a) for(i=0;i<N_c;i++) { a; }
#define CM(f) void f(int m, char *s)
#define CH(f) void f(int m)
#define N "\r\n"
#define N_c 9
#define N_r 3
#define KS ss(m, cs[m]);
enum {
	no, so, we, ea
};
const char *a[]={"quit", "look", "n", "north", "s", "south", "w", "west", "e", "east", "say", "help", "name"};
int c[N_c], cr[N_c], cs[N_c];
char b[N_c][1024];
char cn[N_c][16];
char *rd[N_r]={
	"A large grassy field."N"To the south is a gate"N,
	"A large iron gate."N"To the north is a field, to the south is an ampitheater."N,
	"An abandoned ampitheater."N"To the north is a gate."N,
};
int rp[N_r][4]={
	{-1,1,-1,-1},
	{0,2,-1,-1},
	{1,-1,-1,-1},
};
void wr(int m, const char *s) {
	CK(write(c[m], s, strlen(s)));
}
CH(cl) {
	int i;
	close(c[m]);
	c[m]=0;
	F(if(c[i]&&cr[i]==cr[m]) { wr(i, cn[m]); wr(i, " has disconnected"N); });
}
void ss(int m, int s) {
	cs[m]=s;
	if(s==0) {
		wr(m, ">");
	}
}
CM(c_q) {
	cl(m);
}
CM(c_l) {
	int i, f;
	wr(m, rd[cr[m]]);
	f=0;
	F(if(i!=m&&c[i]&&cr[i]==cr[m]) { wr(m, cn[i]); wr(m, " "); f++; });
	if(f==1) {
		wr(m, "is here"N);
	} else if(f) {
		wr(m, "are here"N);
	}
	KS;
}
void go(int m, int d) {
	int i, t=rp[cr[m]][d];
	if(t<0) {
		wr(m,"You can't go that way"N);
	} else {
		F(if(i!=m&&c[i]&&cr[i]==cr[m]) { wr(i, cn[m]); wr(i, " leaves."N); });
		cr[m]=t;
		F(if(i!=m&&c[i]&&cr[i]==cr[m]) { wr(i, cn[m]); wr(i, " enters."N); });
	}
	c_l(m, "");
}
CM(c_n) {
	go(m, no);
}
CM(c_s) {
	go(m, so);
}
CM(c_w) {
	go(m, we);
}
CM(c_e) {
	go(m, ea);
}
CM(c_y) {
	int i;
	wr(m, "You say \"");
	wr(m, s);
	wr(m, "\""N);
	F(if(m!=i&&c[i]&&cr[i]==cr[m]) { wr(i, cn[m]); wr(i, " says \""); wr(i, s); wr(i, "\""N); });
	KS;
}
CM(c_h) {
	int j;
	for(j=0;j<NR(a);j++) {
		wr(m, a[j]);
		wr(m, " ");
	}
	wr(m, N);
	KS;
}
CM(c_a) {
	int i;
	char *p;
	if(strlen(s)>=S(*cn)) s[S(*cn)-1]=0;
	for(p=s;*p;p++) if(!isalnum(*p)) *p='_';
	if(*s) {
		F(if(i!=m&&c[i]&&cr[i]==cr[m]&&!strncmp(cn[i], s, S(*cn))) { wr(m, "Name already used"N); KS; return; } );

		F(if(i!=m&&c[i]&&cr[i]==cr[m]) { wr(i, cn[m]); wr(i, " is now known as "); wr(i, s); wr(i, N); });
		snprintf(cn[m], S(cn[m]), "%s", s);
		wr(m, "Your name is now \"");
		wr(m, cn[m]);
		wr(m, "\""N);
	}
	KS;
}
CM((*f[NR(a)]))={c_q, c_l, c_n, c_n, c_s, c_s, c_w, c_w, c_e, c_e, c_y, c_h, c_a};
CH(sl) {
	int j, l;
	if(b[m][0]) {
		for(j=0;j<NR(a);j++) {
			l=strcspn(b[m]," ");
			if(l==strlen(a[j]) && !strncmp(b[m], a[j], l)) {
				while(isspace(b[m][l])) l++;
				f[j](m, b[m]+l);
				goto x;
			}
		}
		wr(m, "Try 'help' for commands."N);
	}
	KS;
x:
	b[m][0]=0;
}
CH(nc) {
	b[m][0]=0;
	cr[m]=0;
	cs[m]=0;
	snprintf(cn[m], S(cn[m]), "Someone%u", m+1);
	c_l(m,"");
}
int main(void) {
	int s, i, n, m, t;
	struct sockaddr_in x;
	fd_set r;
	socklen_t l;
	char h[2]={0};
	signal(SIGPIPE, SIG_IGN);
	CK(s=socket(PF_INET, SOCK_STREAM, 0));
	memset(&x, 0, S(x));
	x.sin_port=htons(5555);
	CK(bind(s, &x, S(x)));
	CK(listen(s,1));
	for(;;) {
		FD_ZERO(&r);
		FD_SET(s, &r);
		m=s;
		F(if(c[i]) { FD_SET(c[i], &r); if(c[i]>m) m=c[i]; });
		CK(n=select(m+1, &r, 0, 0, 0));
		if(FD_ISSET(s, &r)) {
			l=S(x);
			CK(t=accept(s, &x, &l));
			F(if(!c[i]) { c[i]=t; nc(i); break; });
			if(i==N_c) {
				close(t);
			}
		}
		F(if(c[i]&&FD_ISSET(c[i], &r)) { CK(t=read(c[i], h, 1)); if(t==0) { cl(i); } else if(*h=='\r') sl(i); else if(*h!='\n') strcat(b[i], h); } );
	}
	return 0;
}
