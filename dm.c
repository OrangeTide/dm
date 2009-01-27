/* dm.c - deathmatch
 * PUBLIC DOMAIN - Jon Mayo - January 26, 2009
 * a tiny unmaintanable telnet game
 */
#include <arpa/inet.h>
#include <ctype.h>
#include <errno.h>
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
#define VER "0.5.2"
#define S(x) sizeof(x)
#define N(x) (S(x)/S(*x))
#define CK(s) if((s)<0) { perror(#s); exit(1); }
#define F(x) for(a=0;a<N(c);a++) { x; }
#define CM(f) void f(int b, char *s)
#define CH(f) void f(int b)
#define CD(f) void f(int b, int d)
#define R "\r\n"
#define KS ss(b, B.s);
#define BI(x,d) (((x)>>d)&1)
#define B c[b]
#define A c[a]
#define Rb r[c[b].r]
#define RET return
#define Iname(i) (id[(i).v].n)
#define Z(p) memset(p, 0, sizeof *p) /* zero */
sig_atomic_t tk_fl=1; /* tick timeout flag */
enum {
	rhand, lhand, head, feet, body,
};
struct ii { /* item instance */
	int v; /* vnum */
	int a;
};
const char *dn[][3]={ /* direction name table and aliases */
	{"north", "n"},
	{"south", "s"},
	{"west", "w"},
	{"east", "e"}
};
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
	{{2}, "pair of rubber boots", feet},
	{{3}, "hunting rifle", feet},
	{{4}, "grannie panties", body},
	{{5}, "rock"},
};
struct {
	int p[N(dn)];
	char *d;
	int e, s;
	struct ii i[9]; /* inventory of the room */
} r[] = {
	{{-1,1,-1,4}, "A large grassy field."R"To the south is a gate, to the east is a parking lot."R, 2, 2, {{1},{3}}},
	{{0,2,-1,-1}, "A large iron gate."R"To the north is a field, to the south is an ampitheater."R, 3, 3, },
	{{1,-1,-1,3}, "An abandoned ampitheater."R"To the north is a gate, to the east is a ticket booth."R, 9, 1},
	{{-1,-1,2,-1}, "Ticket booth."R"West exits to the ampitheater."R, 4, 0, {{4},{2}}},
	{{-1,1,0,-1}, "A anbandoned parking lot."R"To the west is a grassy field."R, 0, 0, {{5}}},
};
void scrub(char *s) { /* remove control characters from a string */
	for(;*s;s++) if(!isprint(*s)) *s='#';
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
	int a, l=b>=0?strlen(B.n):0;
	char u[99];
	va_list ap;
	va_start(ap, s);
	if(b>=0) strcpy(u, B.n);
	vsnprintf(u+l, S(u)-l, s, ap);
	F(if(a!=b&&A.f&&(r==-1||A.r==r)) { wr(a, u); });
}
int ldn(const char *s) {
	int i, j;
	for(i=0;i<N(dn);i++)
		for(j=0;dn[i][j];j++)
			if(!strcasecmp(s, dn[i][j])) RET i;
	RET -1;
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
void inv(int a, int b) {
	char n[99]="You are ";
	int o, f;
	if(a!=b) {
		snprintf(n, S(n), "%s is ", B.n);
	}
	pr(a, "%sweilding %s.\n", n, iname(B.i, rhand));
	pr(a, "%sholding %s.\n", n, iname(B.i, lhand));
	pr(a, "%swearing %s on your head.\n", n, iname(B.i, head));
	pr(a, "%swearing %s on your body.\n", n, iname(B.i, body));
	if(a!=b) {
		pr(a, "%s's pack contains", B.n);
	} else {
		pr(a, "Your pack contains");
	}
	for(f=0,o=body+1;o<N(B.i);o++) {
		if(B.i[o].v) {
			pr(a, " %s", iname(B.i, o));
			f++;
		}
	}
	if(f) {
		wr(a, "."R);
	} else {
		pr(a, " %s."R, id->n);
	}
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
CD(ss) {
	B.s=d;
	if(d==0) {
		wr(b, ">");
	}
}
CM(c_q) {
	cl(b);
}
CM(c_lr) {
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
}
CM(c_l) {
	if(*s) {
		int a;
		F(if(A.f&&A.r==B.r&&!strcasecmp(s, A.n)) { inv(b, a); KS; RET; });
		wr(b, "I don't see that here."R);
	} else {
		c_lr(b, s);
	}
	KS;
}
CD(go) {
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
CM(c_say) {
	int a;
	scrub(s);
	pr(b, "You say \"%s\"."R, s);
	F(if(a!=b&&A.f&&A.r==B.r) { pr(a, "%s says \"%s\""R, B.n, s); });
	KS;
}
CM(c_h);
CM(c_a) {
	int a;
	char *p;
	if(strlen(s)>=S(c->n)) s[S(c->n)-1]=0;
	for(p=s;*p;p++) if(!isalnum(*p)) *p='_';
	if(*s) {
		F(if(a!=b&&A.f&&A.r==B.r&&!strncmp(A.n, s, S(c->n))) { wr(b, "Name already used."R); KS; RET; } );

		sta(b, -1, " is now known as %s."R, s);
		strcpy(B.n, s);
		pr(b, "Your name is now \"%s\"."R, B.n);
	}
	KS;
}
CM(c_o) {
	int d=ldn(s);
	if(d>=0&&BI(Rb.e&Rb.s, d)) {
		int t=Rb.p[d];
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
	int d=ldn(s);
	if(d>=0&&BI(Rb.e, d)&&!BI(Rb.s, d)) {
		int t=Rb.p[d];
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
	F(if(A.f) { n++; pr(b, "%3u %s"R, a+1, A.n); });
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
			wr(b, dn[d][0]);
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
			pr(b, "You hold %s."R, Iname(B.i[o]));
			sta(b, B.r, " holds %s."R, Iname(B.i[o]));
			imv(B.i+lhand, B.i+o);
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
	inv(b, b);
	KS;
}
const struct {
	char *n[4];
	CM((*f));
} ct[] = { /* command table */
	{{"quit"}, c_q},
	{{"look", "l"}, c_l},
	{{"say"}, c_say},
	{{"help", "?"}, c_h},
	{{"name"}, c_a},
	{{"open", "o"}, c_o},
	{{"close", "c"}, c_c},
	{{"who"}, c_wh},
	{{"exits", "ex"}, c_ex},
	{{"take", "t"}, c_t},
	{{"drop", "dr"}, c_dr},
	{{"inventory", "i"}, c_i},
};
CM(c_h) {
	int i, j;
	for(i=0;i<N(ct);i++) {
		for(j=0;ct[i].n[j];j++) {
			wr(b, ct[i].n[j]);
			wr(b, " ");
		}
	}
	wr(b, R);
	KS;
}
CH(sl) {
	int i, j, l;
	if(B.b[0]) {
		if((i=ldn(B.b))>=0) {
			go(b, i);
			goto x;
		}
		l=strcspn(B.b," ");
		if(B.b[l]) B.b[l++]=0;
		while(isspace(B.b[l])) l++;
		for(i=0;i<N(ct);i++) {
			for(j=0;ct[i].n[j];j++) {
				if(!strcmp(B.b, ct[i].n[j])) {
					ct[i].f(b, B.b+l);
					goto x;
				}
			}
		}
		wr(b, "Try 'help' for commands."R);
	}
	KS;
x:
	B.b[0]=0;
}
CD(nc) {
	Z(&B);
	B.f=d;
	snprintf(B.n, S(B.n), "Someone%u", b+1);
	wr(b, "Welcome to Deathmatch!"R"*** version "VER" ***"R"Type 'help' for help."R);
	wh(b);
	c_l(b, "");
}
void sh_tk(int s) { tk_fl=1; } /* signal handler for ticks */
int main(int ac, char **av) {
	int s, a, n, m, t;
	struct sockaddr_in x;
	fd_set z;
	socklen_t l;
	char h[2]={0};
	struct sigaction sa;
	Z(&sa);
	sa.sa_handler=sh_tk;
	sa.sa_flags=SA_RESTART;
	CK(sigaction(SIGALRM, &sa, 0));
	CK(s=socket(PF_INET, SOCK_STREAM, 0));
	Z(&x);
	x.sin_port=htons(ac>1?atoi(av[1]):5555);
	CK(bind(s, &x, S(x)));
	CK(listen(s,1));
	for(;;) {
		FD_ZERO(&z);
		FD_SET(s, &z);
		m=s;
		F(if(A.f) { FD_SET(A.f, &z); if(A.f>m) m=A.f; });
		if(m==s) {
			puts("Zzz"); /* sleeping - no clients */
			tk_fl=-1; /* disable ticks, go to sleep */
			alarm(0);
		} else if(tk_fl<0) {
			tk_fl=1; /* force a tick */
		}
		if(tk_fl>0) { /* process tick */
			/* sta(-1, -1, R"[tick]"R">"); */
			tk_fl=0;
			alarm(5); /* this is the tick rate */
		}
		n=select(m+1, &z, 0, 0, 0);
		if(n<0) {
			if(errno==EINTR) continue;
			CK(-1);
		}
		if(FD_ISSET(s, &z)) {
			l=S(x);
			CK(t=accept(s, &x, &l));
			F(if(!A.f) { nc(a, t); break; }); /* find next free slot */
			if(a==N(c)) {
				close(t);
			}
		}
		F(if(A.f&&FD_ISSET(A.f, &z)) { CK(t=read(A.f, h, 1)); if(t==0) { cl(a); } else if(*h=='\r') sl(a); else if(*h!='\n') strcat(A.b, h); } );
	}
	RET 0;
}
