/****************************************************************************************[Solver.h]
 The MIT License (MIT)

 Copyright (c) 2015, Sam Bayless

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
 OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 **************************************************************************************************/

#ifndef AMOTHEORY_H_
#define AMOTHEORY_H_

#include "monosat/mtl/Vec.h"
#include "monosat/core/SolverTypes.h"
#include "monosat/core/Theory.h"

namespace Monosat {


//At-Most-One theory. This is a special case of PB constraints, for handling at-most-one constraints.
//Each instance of this theory supports a _single_ at-most-one constraint; to implement multiple such constraints, instantiate multiple copies of the theory.
class AMOTheory: public Theory {
	Solver * S;
	int theory_index=-1;

public:

	CRef assign_false_reason;
	//CRef assign_true_reason;

	vec<Var> amo;//list of variables, at most one of which should be true.

	vec<Lit> tmp_clause;

	double propagationtime=0;
	long stats_propagations=0;
	long stats_propagations_skipped=0;
	long stats_shrink_removed = 0;
	long stats_reasons = 0;
	long stats_conflicts = 0;

	Var true_var=var_Undef;
	Var conflict_var=var_Undef;
	bool needs_propagation=false;
	bool clausified = false;
public:

	const char * getTheoryName()const override{
		return "AMO";
	}
	AMOTheory(Solver * S) :
			S(S) {
		S->addTheory(this);
		assign_false_reason=S->newReasonMarker(this);
		//assign_true_reason=S->newReasonMarker(this);

	}
	~AMOTheory() {
	}
	;

	static bool clausify_amo(Solver * S,const vec<Lit> & lits) {
		vec<Lit> set;

		assert(S->decisionLevel() == 0);
		Lit constant_true_lit = lit_Undef;

		for (int i = 0; i < lits.size(); i++) {
			Lit l = lits[i];
			if (S->value(l) == l_False && S->level(var(l)) == 0) {
				//don't add this to the set
			} else if (S->value(l) == l_True && S->level(var(l)) == 0) {
				if (constant_true_lit == lit_Undef) {
					constant_true_lit = l;
				} else {
					//this is a conflict
					S->addClause(~constant_true_lit, ~l);//this is a conflict
					return false;
				}
			} else {
				set.push(l);
			}
		}

		if (constant_true_lit == lit_Undef) {
			for (int i = 0; i < set.size(); i++) {
				for (int j = i + 1; j < set.size(); j++) {
					S->addClause(~set[i], ~set[j]);
				}
			}
		}else{
			//all remaining elements of the set must be false, because constant_true_lit is true.
			for (int i = 0; i < set.size(); i++) {
				S->addClause(~constant_true_lit,~set[i]);//technically don't need to include ~constant_true_lit here, but it might make things cleaner to reason about elsewhere.
				// (It will be eliminated by the solver anyhow, so this doesn't cost anything.)
			}
		}
		return true;
	}


	//Add a variable (not literal!) to the set of which at most one may be true.
	void addVar(Var solverVar){
		S->newTheoryVar(solverVar, getTheoryIndex(),solverVar);//using same variable indices in the theory as out of the theory
		amo.push(solverVar);
	}


	inline int getTheoryIndex()const {
		return theory_index;
	}
	inline void setTheoryIndex(int id) {
		theory_index = id;
	}
	inline void newDecisionLevel() {

	}
	inline void backtrackUntil(int untilLevel){

	}
	inline int decisionLevel() {
		return S->decisionLevel();
	}
	inline void undecideTheory(Lit l){
		if(var(l)==true_var){
			needs_propagation=false;
			true_var=var_Undef;
			assert(conflict_var==var_Undef);
		}
		if(var(l)==conflict_var){
			conflict_var=var_Undef;
		}
	}
	void enqueueTheory(Lit l) {
		if(clausified)
			return;
		if(conflict_var==var_Undef){
			if (!sign(l)){
				if (true_var==var_Undef){
					true_var=var(l);
					assert(!needs_propagation);
					if(opt_amo_eager_prop){
						//enqueue all of the remaining lits in the solver, now.
						stats_propagations++;
						for(Var v:amo){
							if(v!=true_var){
								S->enqueue(mkLit(v,true),assign_false_reason);
							}
						}
					}else{
						needs_propagation=true;
					}
				}else if (var(l)==true_var){
					//we already knew this lit was assigned to true, do nothing.
				}else{
					//there is a conflict - both conflict_var and true_var are assigned true, which is not allowed.
					conflict_var=var(l);
				}
			}else{
				//it is always safe to assign a var to false.
			}
		}
	}
	;
	bool propagateTheory(vec<Lit> & conflict) {
		if (clausified) {
			S->setTheorySatisfied(this);
			return true;
		}
		S->theoryPropagated(this);
		if(decisionLevel()==0){
			//remove constants from the set
			int n_nonconstants = 0;
			bool has_true_lit=false;
			int i,j=0;
			for (i = 0; i < amo.size(); i++) {
				Lit l = mkLit(amo[i]);
				if (S->value(l) == l_False && S->level(var(l)) == 0) {
					//drop this literal from the set
				} else if (S->value(l) == l_True && S->level(var(l)) == 0) {
					amo[j++]=var(l);
					has_true_lit=true;
				} else {
					n_nonconstants++;
					amo[j++]=var(l);
				}
			}
			amo.shrink(i-j);
			if(has_true_lit || amo.size()==0 || amo.size()<=opt_clausify_amo){
				clausified=true;
				vec<Lit> amoLits;
				for(Var v:amo){
					amoLits.push(mkLit(v));
				}
				if(opt_verb>1){
					printf("Clausifying amo theory %d with %d lits\n",this->getTheoryIndex(),amo.size());
				}
				S->setTheorySatisfied(this);
				return clausify_amo(S,amoLits);
			}
		}

		if(conflict_var!=var_Undef){
			conflict.clear();
			assert(true_var!=var_Undef);
			assert(true_var!=conflict_var);
			conflict.push(mkLit(conflict_var,true));
			conflict.push(mkLit(true_var,true));
			needs_propagation=false;
			stats_conflicts++;
			return false;
		}else if (true_var!=var_Undef && needs_propagation){
			stats_propagations++;
			needs_propagation=false;
			assert(!opt_amo_eager_prop);
			//enqueue all of the remaining lits in the solver, now.
			for(Var v:amo){
				if(v!=true_var){
					S->enqueue(mkLit(v,true),assign_false_reason);
				}
			}
		}
		return true;
	}
	void printStats(int detailLevel) {
		if(!clausified) {
			printf("AMO Theory %d stats:\n", this->getTheoryIndex());

			printf("Propagations: %ld (%f s, avg: %f s, %ld skipped)\n", stats_propagations, propagationtime,
				   (propagationtime) / ((double) stats_propagations + 1), stats_propagations_skipped);

			printf("Conflicts: %ld\n", stats_conflicts);
			printf("Reasons: %ld\n", stats_reasons);

			fflush(stdout);
		}
	}

	inline bool solveTheory(vec<Lit> & conflict){
		return propagateTheory(conflict);
	}
	inline void buildReason(Lit p, vec<Lit> & reason, CRef reason_marker){
		stats_reasons++;
		assert(reason_marker==assign_false_reason);
		if(var(p)!=true_var){
			assert(sign(p));
			assert(S->value(p)==l_True);
			assert(S->value(true_var)==l_True);
			reason.push(mkLit( var(p),true));
			reason.push(mkLit(true_var,true));//either true_var (currently assigned true) must be false, or var(p) must be false
		}else{
			assert(false);
		}
	}
	bool check_solved() {
		int n_true=0;
		for (Var v:amo){
			if(S->value(v)==l_True){
				n_true+=1;
				if(n_true>1){
					return false;
				}
			}
		}

		return true;
	}
private:


};

}
;

#endif /* AMOTheory_H_ */
