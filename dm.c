/* dm.c - deathmatch
 * PUBLIC DOMAIN - Jon Mayo - January 26, 2009
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
#define V "0.5.1"
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
#define Iname(i) (id[(i).v].n)
enum {
	rhand, lhand, head, body,
};
struct ii { /* item instance */
	int v; /* vnum */
	int a;
};
const char *dn[]={"north", "south", "west", "east"};
const char *q[]={"quit", "look", "n", "north", "s", "south", "w", "west", "e", "east", "say", "help", "name", "open", "o", "close", "c", "who", "exits", "take", "drop", "inventory", "i" };
struct {
	int f, r, s; /* file descriptor, current room, state */
	char n[16], b[999]; /* name and buffer */
	struct ii i[9]; /* inventory */
} c[9];
const struct { /* item definitions */
	struct ii i; /* initial */
	char *n;
	int wl; /* wear location */
} id[] = {
	{{0}, "nothing"},
	{{1}, "coonskin cap", head},
};
struct {
	int p[N(dn)];
	char *d;
	int e, s;
	struct ii i[9]; /* inventory of the room */
} r[] = {
	{{-1,1,-1,4}, "A large grassy field."R"To the south is a gate, to the east is a parking lot."R, 2, 2, {{1}}},
	{{0,2,-1,-1}, "A large iron gate."R"To the north is a field, to the south is an ampitheater."R, 3, 3, },
	{{1,-1,-1,3}, "An abandoned ampitheater."R"To the north is a gate, to the east is a ticket booth."R, 9, 1},
	{{-1,-1,2,-1}, "Ticket booth."R"West exits to the ampitheater."R, 4, 0},
	{{-1,1,0,-1}, "A anbandoned parking lot."R"To the west is a grassy field."R, 0, 0},
};
void scrub(char *s) { /* remove control characters from a string */
	while(*s) if(!isprint(*s)) *s='#';
}
void wr(int b, const char *s) {
	if(B.f) CK(write(B.f, s, strlen(s)));
}
void pr(int b, const char *s, ...) {
	char u[99];
	va_list ap;
	va_start(ap, s);
	vsnprintf(u, S(u), s, ap);
	va_end(ap);
	wr(b, u);
}
void sta(int b, int r, const char *s, ...) {
	int a, l=strlen(B.n);
	char u[99];
	va_list ap;
	va_start(ap, s);
	strcpy(u, B.n);
	vsnprintf(u+l, S(u)-l, s, ap);
	F(if(a!=b&&A.f&&(r==-1||A.r==r)) { wr(a, u); });
}
int ldn(const char *s) {
	int i;
	for(i=0;i<N(dn);i++)
		if(!strcasecmp(s, dn[i])) RET i;
	RET 0;
}
char *iname(struct ii *ii, int o) {
	static char b[99];
	char *s=Iname(ii[o]);
	if(ii[o].v) {
		snprintf(b, S(b), "%s %s", strchr("aeiou", *s)?"an":"a", s);
	} else {
		strcpy(b, id->n);
	}
	RET b;
}
int ikw(int n, struct ii *ii, const char *k) { /* find first keyword match */
	int i;
	for(i=0;i<n;i++) {
		if(ii[i].v&&!strcasecmp(Iname(ii[i]), k)) return i;
	}
	return -1;
}
int inf(int n, struct ii *ii, int skip) { /* return next free inventory slot */
	int i;
	for(i=skip;i<n;i++) {
		if(!ii[i].v) return i;
	}
	return -1;
}
void imv(struct ii *d, struct ii *s) { /* move inventory item from s to d */
	*d=*s;
	s->v=0;
}
int idr(int b, struct ii *s) { /* drop item s to room that b is standing in */
	int d;
	d=inf(N(Rb.i), Rb.i, 0);
	if(d<0) { /* room inventory is full */
		wr(b, "No room on the floor."R);
		RET 0;
	}
	pr(b, "You drop %s."R, Iname(*s));
	sta(b, B.r, " drops %s."R, Iname(*s));
	imv(Rb.i+d, s);
	RET 1;
}
CH(cl) {
	int o, d;
	close(B.f);
	B.f=0;
	sta(b, -1, " has disconnected."R);
	/* TODO: drop all the inventory items */
	for(o=0;o<N(B.i);o++) {
		if(B.i[o].v) {
			d=inf(N(Rb.i), Rb.i, 0);
			if(d<0) {
				sta(b, -1, "'s %s was destroyed."R, iname(B.i, o));
				B.i[o].v=0;
			} else {
				idr(b, B.i+o);
			}
		}
	}
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
	int a, f, i;
	wr(b, Rb.d);
	/* check for items on the floor */
	for(f=0,i=0;i<N(Rb.i);i++) {
		if(Rb.i[i].v) {
			f++;
			pr(b, "%s ", iname(Rb.i, i));
		}
	}
	if(f==1) {
		wr(b, "is laying here."R);
	} else if(f) {
		wr(b, "are laying here."R);
	}
	/* check for players */
	f=0;
	F(if(a!=b&&A.f&&A.r==B.r) { wr(b, A.n); wr(b, " "); f++; });
	if(f==1) {
		wr(b, "is here."R);
	} else if(f) {
		wr(b, "are here."R);
	}
	KS;
}
void go(int b, int d) {
	int t=Rb.p[d];
	if(t<0) {
		wr(b,"You can't go that way!"R);
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
	go(b, 0); /* north */
}
CM(c_s) {
	go(b, 1); /* south */
}
CM(c_w) {
	go(b, 2); /* west */
}
CM(c_e) {
	go(b, 3); /* east */
}
CM(c_say) {
	int a;
	scrub(s);
	pr(b, "You say \"%s\"."R, s);
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
		F(if(a!=b&&A.f&&A.r==B.r&&!strncmp(A.n, s, S(c->n))) { wr(b, "Name already used."R); KS; RET; } );

		sta(b, -1, " is now known as %s."R, B.n);
		strcpy(B.n, s);
		pr(b, "Your name is now \"%s\"."R, B.n);
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
CM(c_t) {
	int fr=0, d, o=ikw(N(Rb.i), Rb.i, s);
	if(o<0) { o=ikw(N(B.i), B.i, s); fr=1; }
	if(o<0) {
		wr(b, "That is not here."R);
	} else {
		if(B.i[lhand].v) { /* clear out the left hand */
			d=inf(N(B.i), B.i, body+1);
			if(d<0) { /* inventory full */
				wr(b, "Your inventory is full!"R);
				KS;
				RET;
			}
			pr(b, "You put away %s."R, Iname(B.i[lhand]));
			sta(b, B.r, " puts away %s."R, Iname(B.i[lhand]));
			imv(B.i+d, B.i+lhand);
		}
		if(fr) {
			imv(B.i+lhand, B.i+o);
			pr(b, "You hold %s."R, Iname(B.i[o]));
			sta(b, B.r, " holds %s."R, Iname(B.i[o]));
		} else {
			pr(b, "You pick up %s."R, Iname(Rb.i[o]));
			sta(b, B.r, " picks up %s."R, Iname(Rb.i[o]));
			imv(B.i+lhand, Rb.i+o);
		}
	}
	KS;
}
CM(c_dr) { /* "drop" command */
	int o=ikw(N(B.i), B.i, s);
	if(o<0) {
		wr(b, "That is not here."R);
	} else if(!idr(b, B.i+o)) {
		wr(b, "No space on the floor."R);
	}
	KS;
}
CM(c_i) {
	int o, f;
	pr(b, "You are weilding %s.\n", iname(B.i, rhand));
	pr(b, "You are holding %s.\n", iname(B.i, lhand));
	pr(b, "You are wearing %s on your head.\n", iname(B.i, head));
	pr(b, "You are wearing %s on your body.\n", iname(B.i, body));
	pr(b, "Your pack contains");
	for(f=0,o=body+1;o<N(B.i);o++) {
		if(B.i[o].v) {
			pr(b, " %s", iname(B.i, o));
			f++;
		}
	}
	if(f) {
		wr(b, "."R);
	} else {
		pr(b, " %s."R, id->n);
	}
	KS;
}
CM((*f[N(q)]))={c_q, c_l, c_n, c_n, c_s, c_s, c_w, c_w, c_e, c_e, c_say, c_h, c_a, c_o, c_o, c_c, c_c, c_wh, c_ex, c_t, c_dr, c_i, c_i};
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
	memset(B.i, 0, S(B.i));
	snprintf(B.n, S(B.n), "Someone%u", b+1);
	wr(b, "Welcome to Deathmatch!"R"*** version "V" ***"R"Type 'help' for help."R);
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
