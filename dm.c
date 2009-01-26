/* dm.c - deathmatch
 * PUBLIC DOMAIN - Jon Mayo - January 25, 2009
 * a tiny unmaintanable telnet game
 */
#include <arpa/inet.h>
#include <ctype.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>
#define V "v0.5alpha"
#define S(x) sizeof(x)
#define N(x) (S(x)/S(*x))
#define CK(s) if((s)<0) { perror(#s); exit(1); }
#define F(x) for(a=0;a<N(c);a++) { x; }
#define CM(f) void f(int b, char *s)
#define CH(f) void f(int b)
#define R "\r\n"
#define KS ss(b, B.s);
#define BI(x,d) (((x)>>d)&1)
#define B c[b]
#define A c[a]
#define Rb r[c[b].r]
#define RET return
enum {
	no, so, we, ea
};
const char *dn[]={"north", "south", "west", "east"};
const char *q[]={"quit", "look", "n", "north", "s", "south", "w", "west", "e", "east", "say", "help", "name", "open", "o", "close", "c", "who", "exits" };
struct {
	int f, r, s;
	char n[16], b[999];
} c[9];
struct {
	int p[N(dn)];
	char *d;
	int e, s;
} r[] = {
	{{-1,1,-1,4}, "A large grassy field."R"To the south is a gate, to the east is a parking lot."R, 2, 2},
	{{0,2,-1,-1}, "A large iron gate."R"To the north is a field, to the south is an ampitheater."R, 3, 3},
	{{1,-1,-1,3}, "An abandoned ampitheater."R"To the north is a gate, to the east is a ticket booth."R, 9, 1},
	{{-1,-1,2,-1}, "Ticket booth."R"West exits to the ampitheater."R, 4, 0},
	{{-1,1,0,-1}, "A anbandoned parking lot."R"To the west is a grassy field."R, 0, 0},
};
void wr(int b, const char *s) {
	CK(write(B.f, s, strlen(s)));
}
void pr(int b, const char *s, ...) {
	char u[99];
	va_list ap;
	va_start(ap, s);
	vsnprintf(u, S(u), s, ap);
	va_end(ap);
	wr(b, u);
}
void sta(int b, int r, const char *s) {
	int a;
	F(if(a!=b&&A.f&&A.r==r) { wr(a, B.n); wr(a, s); });
}
int ldn(const char *s) {
	int i;
	for(i=0;i<N(dn);i++)
		if(!strcmp(s, dn[i])) RET i;
	RET 0;
}
CH(cl) {
	close(B.f);
	B.f=0;
	sta(b, B.r, " has disconnected"R);
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
	wr(b, Rb.d);
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
	int t=Rb.p[d];
	if(t<0) {
		wr(b,"You can't go that way"R);
	} else if(BI(Rb.e&Rb.s,d)) {
		wr(b,"The door is closed."R);
	} else {
		B.r=t;
		sta(b, B.r," leaves."R);
		sta(b, t, " enters."R);
		c_l(b, "");
		RET;
	}
	KS;
	RET;
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
	pr(b, "You say \"%s\""R, s);
	F(if(a!=b&&A.f&&A.r==B.r) { pr(a, "%s says \"%s\""R, B.n, s); });
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
		F(if(a!=b&&A.f&&A.r==B.r&&!strncmp(A.n, s, S(c->n))) { wr(b, "Name already used"R); KS; RET; } );

		F(if(a!=b&&A.f&&A.r==B.r) { pr(a, "%s is now known as %s"R, B.n, s); });
		strcpy(B.n, s);

		pr(b, "Your name is now \"%s\""R, B.n);
	}
	KS;
}
CM(c_o) {
	int d=ldn(s), t=Rb.p[d];
	if(BI(Rb.e&Rb.s, d)) {
		Rb.s^=1<<d;
		r[t].s^=1<<(d^1);
		wr(b, "You open the exit."R);
		sta(b, B.r, " opens the exit."R);
		sta(b, t, " opens the exit."R);
	} else {
		wr(b, "You can't open that."R);
	}
	KS;
}
CM(c_c) {
	int d=ldn(s), t=Rb.p[d];
	if(BI(Rb.e, d)&&!BI(Rb.s, d)) {
		Rb.s^=1<<d;
		r[t].s^=1<<(d^1);
		wr(b, "You close the exit."R);
		sta(b, B.r, " closes the exit."R);
		sta(b, t, " closes the exit."R);
	} else {
		wr(b, "You can't close that."R);
	}
	KS;
}
void wh(int b) {
	int a, n=0;
	F(if(A.f) { n++; pr(b, "%3u %s"R, a, A.n); });
	pr(b, "%u users online."R"***"R, n);
}
CM(c_wh) {
	wh(b);
	KS;
}
CM(c_ex) {
	int d, e;
	for(d=0;d<N(Rb.p);d++) {
		if((e=Rb.p[d])>=0) {
			wr(b, dn[d]);
			if(BI(Rb.e, d)) wr(b, BI(Rb.s, d)?"(opened)":"(closed)");
			wr(b, " ");
		}
	}
	wr(b, R);
	KS;
}
CM((*f[N(q)]))={c_q, c_l, c_n, c_n, c_s, c_s, c_w, c_w, c_e, c_e, c_y, c_h, c_a, c_o, c_o, c_c, c_c, c_wh, c_ex};
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
	wr(b, "Welcome to Deathmatch!"R"*** "V" ***"R"Type 'help' for help."R);
	wh(b);
	c_l(b, "");
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
	RET 0;
}
