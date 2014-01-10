
#ifndef CONNECTIVITY_H_
#define CONNECTIVITY_H_

#include "mtl/Vec.h"
#include "mtl/Heap.h"
#include "DynamicGraph.h"
#include "core/Config.h"
#include "Reach.h"
using namespace Minisat;



template<class Status,class EdgeStatus=DefaultEdgeStatus>
class FloydWarshall:public Reach{
public:

	DynamicGraph<EdgeStatus> & g;

	int last_modification;
	int last_addition;
	int last_deletion;
	int history_qhead;

	int last_history_clear;

	vec<Status*>   status;
	vec<int> sources;
	int INF;


	vec<int> q;
	vec<int> check;
	const int reportPolarity;

	vec<vec<int> > dist;
	vec<vec<int> >  next;

	struct DefaultReachStatus{
			vec<bool> stat;
				void setReachable(int u, bool reachable){
					stat.growTo(u+1);
					stat[u]=reachable;
				}
				bool isReachable(int u) const{
					return stat[u];
				}
				DefaultReachStatus(){}
			};

public:


	FloydWarshall(DynamicGraph<EdgeStatus> & graph,  int _reportPolarity=0 ):g(graph), status(_status), last_modification(-1),last_addition(-1),last_deletion(-1),history_qhead(0),last_history_clear(0),INF(0),reportPolarity(0){
		marked=false;
		mod_percentage=0.2;
		stats_full_updates=0;
		stats_fast_updates=0;
		stats_skip_deletes=0;
		stats_skipped_updates=0;
		stats_full_update_time=0;
		stats_fast_update_time=0;
		stats_num_skipable_deletions=0;
		stats_fast_failed_updates=0;
	}


	void addSource(int s,Status * _status){
		assert(!sources.contains(s));
		sources.push(s);
		status.push(_status);
		last_modification=-1;
		last_addition=-1;
		last_deletion=-1;
	}
	void setSource(int s){

		}
	int getSource(){
		return -1;
	}



	void setNodes(int n){
		q.capacity(n);
		check.capacity(n);
		seen.growTo(n);

		INF=g.nodes+1;
		dist.growTo(n);
		for(int i =0;i<dist.size();i++)
			dist[i].growTo(n);

		next.growTo(n);
		for(int i =0;i<dist.size();i++)
			next[i].growTo(n);
	}


	void update( ){
		static int iteration = 0;
		int local_it = ++iteration ;
		stats_full_updates++;
		double startdupdatetime = cpuTime();
		if(last_modification>0 && g.modifications==last_modification){
			stats_skipped_updates++;
			return;
		}

		if(last_deletion==g.deletions){
			stats_num_skipable_deletions++;
		}

		setNodes(g.nodes);

		q.clear();
		for(int i = 0;i<g.nodes;i++){
			next[i]=-1;
			dist[i][i]=0;
		}

		for(int i = 0;i<g.all_edges.size();i++){
			 int u =g.all_edges[i].from;
			 int v =g.all_edges[i].to;
			 dist[u][v]= 1;
		}

		for(int l = 0;l<sources.size();l++){
			int k = sources[l];
			for(int i = 0;i<g.nodes;i++){
				for (int j = 0;j<g.nodes;j++){
					int d = dist[i][k] + dist[k][j];
					if(d < dist[i][j]){
						dist[i][j] = d;
						next[i][j]=k;
					}
				}
			}
		}
		for(int i = 0;i<sources.size();i++){
			int s = sources[i];
			for(int u = 0;u<g.nodes;u++){
				if(!seen[u] && reportPolarity<1){
					status[s]->setReachable(u,false);
				}else 	if(seen[u] && reportPolarity>-1){
					status[s]->setReachable(u,true);
				}
			}
		}
		assert(dbg_uptodate());

		last_modification=g.modifications;
		last_deletion = g.deletions;
		last_addition=g.additions;

		history_qhead=g.history.size();
		last_history_clear=g.historyclears;

		stats_full_update_time+=cpuTime()-startdupdatetime;;
	}

	void path(int from, int to, vec<int> & path){
		assert(dist[from][to]<INF);
		int intermediate = next[from][to];
		path(from, intermediate, path);
		path.push(intermediate);
		path(intermediate,to);
	}

	bool dbg_path(int from,int to){

		return true;
	}
	void drawFull(){
				printf("digraph{\n");
				for(int i = 0;i< g.nodes;i++){

					if(seen[i]){
						printf("n%d [fillcolor=blue style=filled]\n", i);
					}else{
						printf("n%d \n", i);
					}


				}

				for(int i = 0;i< g.adjacency.size();i++){
					for(int j =0;j<g.adjacency[i].size();j++){
					int id  =g.adjacency[i][j].id;
					int u =  g.adjacency[i][j].node;
					const char * s = "black";
					if( g.edgeEnabled(id))
						s="blue";
					else
						s="red";



					printf("n%d -> n%d [label=\"v%d\",color=\"%s\"]\n", i,u, id, s);
					}
				}

				printf("}\n");
			}
	bool dbg_uptodate(){

		return true;
	}

	bool connected_unsafe(int t)const{
		return t<seen.size() && seen[t];
	}
	bool connected_unchecked(int t)const{
		assert(last_modification==g.modifications);
		return connected_unsafe(t);
	}
	bool connected(int t){
		if(last_modification!=g.modifications)
			update();

		assert(dbg_uptodate());

		return seen[t];
	}
	int distance(int t){
		if(connected(t))
			return 1;
		else
			return INF;
	}
	int distance_unsafe(int t){
		if(connected_unsafe(t))
			return 1;
		else
			return INF;
	}
	int previous(int t){
		assert(t>=0 && t<next.size());
		assert(next[t]>=-1 && next[t]<next.size());
		return next[t];
	}

};

#endif
