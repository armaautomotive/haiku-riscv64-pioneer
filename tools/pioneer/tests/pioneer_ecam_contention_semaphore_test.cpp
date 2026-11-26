/* SPDX-License-Identifier: MIT */
#include <cassert>
#include <cstdio>
using sem_id=int;using thread_id=int;using status_t=int;using team_id=int;
enum {B_OK=0,B_ERROR=-1};
struct thread_info {team_id team;};
struct sem_info {team_id team;int count;};
static sem_info sems[]={{7,0},{7,0},{8,0}};
static thread_id find_thread(const char*) {return 4;}
static status_t get_thread_info(thread_id,thread_info* info) {info->team=7;return B_OK;}
static status_t get_sem_info(sem_id id,sem_info* info) {if(id<0 || id>=3)return B_ERROR;*info=sems[id];return B_OK;}
#include "pioneer_ecam_contention_semaphores.inc"
int main(){
 assert(safe_semaphores(0,1));
 sems[0].count=-1;assert(safe_semaphores(0,1));
 sems[0].count=-2;assert(!safe_semaphores(0,1));
 sems[0].count=1;assert(!safe_semaphores(0,1));
 sems[0].count=0;sems[1].count=1;assert(!safe_semaphores(0,1));
 sems[1].count=0;assert(!safe_semaphores(0,0) && !safe_semaphores(-1,1));
 assert(!safe_semaphores(0,2) && !safe_semaphores(0,3));
 std::puts("PASS actual contention semaphore ownership and initial-state checks; SIMULATED ONLY");
}
