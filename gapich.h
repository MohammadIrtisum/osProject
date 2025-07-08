#ifndef GAPICH_H
#define GAPICH_H

#include <graphics.h>
#include <bits/stdc++.h>
#include "ui.h"

using namespace std;

/*****************************************************************************************
 *  gapich.h — Scheduling algorithms GUI helpers                                          *
 *                                                                                        *
 *  Four high‑level entry points are provided. Just declare them at global scope (or      *
 *  include this header after function prototypes in function.h) and call them from the   *
 *  menu:                                                                                 *
 *      • rr_input()           – collect data & animate Round‑Robin Gantt chart           *
 *      • rr_visualization()   – queue‑level RR visualisation                             *
 *      • sjf_input()          – collect data & animate SJF (pre‑emptive) Gantt chart     *
 *      • sjf_visualization()  – (currently alias for sjf_input, keep for symmetry)       *
 *                                                                                        *
 *  Coordinates and drawing helpers assume the same 800×520 window and the helper UI      *
 *  classes (Field, Button, Input) that already exist in your project.                    *
 *****************************************************************************************/

extern void frame();   // already defined in function.h
extern void Footer();  // –»
extern void menu();    // –»

/*────────────────────────────────────────────────────────────────────────────────────────*/
struct Segment {
    int pid;    // process id (‑1 ⇒ idle)
    int start;  // inclusive
    int end;    // exclusive
};

static int colors[] = { LIGHTRED, LIGHTGREEN, LIGHTBLUE, LIGHTCYAN,
                        LIGHTMAGENTA, YELLOW, WHITE, LIGHTGRAY };

int getColor(int pid){ return (pid == -1 ? DARKGRAY : colors[(pid-1) % 8]); }

/* Parse string like "0 5,1 3,2 8,3 6"  →  {{0,5,1},{1,3,2},...} */
vector<array<int,3>> parseProcessList(const string &s){
    vector<array<int,3>> res;  int at, bt, id = 1;  char comma;
    stringstream ss(s);
    while(ss >> at >> bt){ res.push_back({at,bt,id++}); ss >> comma; }
    return res;
}

/*──────────────── Round‑Robin core ───────────────*/
vector<array<int,3>> roundRobinSchedule(vector<array<int,3>> v, int q){
    sort(v.begin(), v.end());
    int n = v.size();
    vector<int> rem(n);
    for(int i=0;i<n;i++) rem[i] = v[i][1];

    queue<int> Q;  vector<array<int,3>> seg;  int idx = 0, t = 0;

    while(!Q.empty() || idx < n){
        while(idx < n && v[idx][0] <= t){ Q.push(idx++); }
        if(Q.empty()){ t = v[idx][0]; continue; }

        int i = Q.front(); Q.pop();
        int exec = min(q, rem[i]);
        seg.push_back({v[i][2], t, t+exec});
        t += exec;  rem[i] -= exec;

        while(idx < n && v[idx][0] <= t){ Q.push(idx++); }
        if(rem[i] > 0) Q.push(i);
    }
    return seg;
}

/*──────────────── SJF (pre‑emptive, 1‑unit granularity) ───────────────*/
vector<array<int,3>> sjfSchedule(vector<array<int,3>> v){
    sort(v.begin(), v.end());
    int n = v.size(), completed = 0, t = v.front()[0];
    vector<int> rem(n); for(int i=0;i<n;i++) rem[i] = v[i][1];

    vector<array<int,3>> seg;  int last = -2;
    while(completed < n){
        int best = -1, bestRem = INT_MAX;
        for(int i=0;i<n;i++) if(v[i][0] <= t && rem[i] && rem[i] < bestRem){ bestRem = rem[i]; best = i; }

        if(best == -1){ t++; last = -1; continue; }
        if(best != last) seg.push_back({v[best][2], t, t+1});
        else seg.back()[2] = t+1;

        rem[best]--; if(!rem[best]) completed++;  last = best;  t++;
    }
    return seg;
}

/*──────────────── Generic Gantt animator ───────────────*/
void animateSchedule(const vector<array<int,3>> &seg, const string &title){
    cleardevice(); frame(); Footer();
    settextstyle(6,0,3);
    outtextxy(400 - textwidth((char*)title.c_str())/2, 40, (char*)title.c_str());

    if(seg.empty()) return;
    int tot = seg.back()[2];
    double unit = 748.0 / tot;   // 16px margin both sides
    int baseY = 200, h = 40;

    for(auto s : seg){
        int l = 16 + int(unit * s[1]);
        int r = 16 + int(unit * s[2]);
        setfillstyle(SOLID_FILL, getColor(s[0]));
        bar(l, baseY, r, baseY+h);
        setbkcolor(getColor(s[0])); setcolor(BLACK);
        char lbl[10]; sprintf(lbl, "%s", (s[0]==-1?"IDLE":("P"+to_string(s[0])).c_str()));
        outtextxy((l+r)/2 - textwidth(lbl)/2, baseY+h/2 - textheight("A")/2, lbl);
        delay(400);
    }

    // timeline
    setcolor(BLACK); line(16, baseY+h+10, 784, baseY+h+10);
    for(int t=0;t<=tot;t++){
        int x = 16 + int(unit*t); line(x, baseY+h+7, x, baseY+h+13);
        char txt[6]; sprintf(txt, "%d", t); outtextxy(x-textwidth(txt)/2, baseY+h+15, txt);
    }

    Button back(15,425,130,480, BLUE, "BACK");
    while(true){
        back.hover(GREEN);
        if(GetAsyncKeyState(VK_LBUTTON) & (0x8000 != 0)) if(back.cursor()){ menu(); return; }
        if(kbhit()) getch();
    }
}

/*───────────────── Round‑Robin input → animation ───────────────────*/
void rr_input(){
    cleardevice(); frame(); Footer();
    settextstyle(8,0,2);
    new Field(50,80,750,120,CYAN,BLACK,"Enter processes as: AT BT,AT BT,...");
    new Field(50,140,300,190,GREEN,WHITE,"Processes");
    Input procBox; procBox.Name(300,140,750,190);

    new Field(50,210,300,260,GREEN,WHITE,"Time Quantum");
    Input qBox; qBox.Name(300,210,750,260);

    Button run(350,300,450,350, BLUE, "RUN");  Button back(15,425,130,480, BLUE, "BACK");
    char procStr[500]="", qStr[20]="";

    while(true){
        run.hover(GREEN); back.hover(GREEN);
        if(GetAsyncKeyState(VK_LBUTTON) & (0x8000 != 0)){
            if(procBox.cursor()) procBox.getName(procStr);
            else if(qBox.cursor()) qBox.getName(qStr);
            else if(run.cursor()){
                int q = atoi(qStr); vector<array<int,3>> v = parseProcessList(procStr);
                if(q<=0 || v.empty()) { outtextxy(50,320,"Invalid input!"); continue; }
                animateSchedule(roundRobinSchedule(v,q), "Round Robin – Gantt Chart"); return; }
            else if(back.cursor()) { menu(); return; }
        }
    }
}

/*────────── RR visualisation with live queue ─────────*/
void rr_visualization(){
    cleardevice(); frame(); Footer();
    settextstyle(8,0,2);
    new Field(50,80,750,120,CYAN,BLACK,"Enter processes as: AT BT,AT BT,...");
    new Field(50,140,300,190,GREEN,WHITE,"Processes");
    Input procBox; procBox.Name(300,140,750,190);

    new Field(50,210,300,260,GREEN,WHITE,"Time Quantum");
    Input qBox; qBox.Name(300,210,750,260);

    Button run(300,300,500,350, BLUE, "VISUALIZE");  Button back(15,425,130,480, BLUE, "BACK");
    char procStr[500]="", qStr[20]="";

    while(true){
        run.hover(GREEN); back.hover(GREEN);
        if(GetAsyncKeyState(VK_LBUTTON) & (0x8000 != 0)){
            if(procBox.cursor()) procBox.getName(procStr);
            else if(qBox.cursor()) qBox.getName(qStr);
            else if(run.cursor()){
                int q = atoi(qStr); vector<array<int,3>> v = parseProcessList(procStr);
                if(q<=0 || v.empty()){ outtextxy(50,320,"Invalid input!"); continue; }

                sort(v.begin(), v.end());
                int n=v.size(); vector<int> rem(n); for(int i=0;i<n;i++) rem[i]=v[i][1];
                queue<int> Q; int idx=0, t=0;

                cleardevice(); frame(); Footer();
                settextstyle(6,0,3);
                outtextxy(400 - textwidth("Round Robin – Live Queue")/2, 40, "Round Robin – Live Queue");

                while(!Q.empty() || idx<n){
                    while(idx<n && v[idx][0]<=t) Q.push(idx++);

                    // queue box
                    setfillstyle(SOLID_FILL, WHITE); bar(100,120,700,160);
                    rectangle(100,120,700,160); setcolor(BLACK);
                    int pos=110; queue<int> tmp=Q;
                    while(!tmp.empty()){ char lbl[5]; sprintf(lbl,"P%d",v[tmp.front()][2]); outtextxy(pos,130,lbl); pos+=textwidth(lbl)+20; tmp.pop(); }

                    // CPU box
                    setfillstyle(SOLID_FILL, LIGHTGRAY); bar(350,200,450,260); rectangle(350,200,450,260);
                    char timeLbl[10]; sprintf(timeLbl,"t=%d",t); outtextxy(355,235,timeLbl);

                    if(Q.empty()){ outtextxy(365,215,"IDLE"); delay(400); t++; continue; }

                    int i=Q.front(); Q.pop(); int exec=min(q,rem[i]);
                    char lbl[6]; sprintf(lbl,"P%d",v[i][2]); setbkcolor(LIGHTGRAY); outtextxy(375-textwidth(lbl)/2,215,lbl);
                    delay(400*exec);
                    rem[i]-=exec; t+=exec;
                    while(idx<n && v[idx][0]<=t) Q.push(idx++);
                    if(rem[i]>0) Q.push(i);
                }
                Button back2(15,425,130,480, BLUE, "BACK");
                while(true){ back2.hover(GREEN); if(GetAsyncKeyState(VK_LBUTTON)&(0x8000!=0)) if(back2.cursor()){ menu(); return; } if(kbhit()) getch(); }
            }
            else if(back.cursor()){ menu(); return; }
        }
    }
}

/*───────────────── SJF input & animation ──────────────*/
void sjf_input(){
    cleardevice(); frame(); Footer();
    settextstyle(8,0,2);
    new Field(50,80,750,120,CYAN,BLACK,"Enter processes as: AT BT,AT BT,...");
    new Field(50,140,300,190,GREEN,WHITE,"Processes");
    Input procBox; procBox.Name(300,140,750,190);

    Button run(350,250,450,300, BLUE, "RUN");  Button back(15,425,130,480, BLUE, "BACK");
    char procStr[500]="";

    while(true){
        run.hover(GREEN); back.hover(GREEN);
        if(GetAsyncKeyState(VK_LBUTTON) & (0x8000 != 0)){
            if(procBox.cursor()) procBox.getName(procStr);
            else if(run.cursor()){
                vector<array<int,3>> v=parseProcessList(procStr);
                if(v.empty()){ outtextxy(50,320,"Invalid input!"); continue; }
                animateSchedule(sjfSchedule(v), "SJF – Gantt Chart"); return; }
            else if(back.cursor()){ menu(); return; }
        }
    }
}

void sjf_visualization(){ sjf_input(); }

#endif /* GAPICH_H */

