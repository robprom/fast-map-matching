/**
 * Fast map matching.
 *
 * Definition of a TransitionGraph.
 *
 * @author: Can Yang
 * @version: 2020.01.31
 */

#include "mm/transition_graph.hpp"
#include "network/type.hpp"
#include "util/debug.hpp"

using namespace FMM;
using namespace FMM::CORE;
using namespace FMM::NETWORK;
using namespace FMM::MM;

TransitionGraph::TransitionGraph(const Traj_Candidates &tc, double gps_error){
  for (auto cs = tc.begin(); cs!=tc.end(); ++cs) {
    TGLayer layer;
    for (auto iter = cs->begin(); iter!=cs->end(); ++iter) {
      double ep = calc_ep(iter->dist,gps_error);
      layer.push_back(TGNode{&(*iter),nullptr,ep,0});
    }
    layers.push_back(layer);
  }
  if (!tc.empty()) {
    reset_layer(&(layers[0]));
  }
}

double TransitionGraph::calc_tp(double sp_dist,double eu_dist){
  return eu_dist>=sp_dist ? (sp_dist+1e-6)/(eu_dist+1e-6) : eu_dist/sp_dist;
}

double TransitionGraph::calc_ep(double dist,double error){
  double a = dist / error;
  return exp(-0.5 * a * a);
}

// Reset the properties of a candidate set
void TransitionGraph::reset_layer(TGLayer *layer){
  for (auto iter=layer->begin(); iter!=layer->end(); ++iter) {
    iter->cumu_prob = iter->ep;
    iter->prev = nullptr;
  }
}

const TGNode *TransitionGraph::find_optimal_candidate(const TGLayer &layer){
  const TGNode *opt_c=nullptr;
  double final_prob = -0.001;
  for (auto c = layer.begin(); c!=layer.end(); ++c) {
    if(final_prob < c->cumu_prob) {
      final_prob = c->cumu_prob;
      opt_c = &(*c);
    }
  }
  return opt_c;
};

TGOpath TransitionGraph::backtrack(){
  SPDLOG_TRACE("Backtrack start");
  TGOpath opath;
  TGNode *opt_c=nullptr;
  double final_prob = -0.001;
  int N = layers.size();
  for (int i=N-1; i>=0; --i) {
    opt_c = find_optimal_candidate(layers[i]);
    opath.push_back(opt_c);
    if (opt_c!=nullptr) {
      while ((opt_c=opt_c->prev)!=nullptr)
      {
        opath.push_back(opt_c);
        --i;
      }
    }
  }
  std::reverse(opath.begin(), opath.end());
  SPDLOG_TRACE("Backtrack done");
  return opath;
}

std::vector<TGLayer> &TransitionGraph::get_layers(){
  return layers;
}

MatchedCandidatePath TransitionGraph::extract_matched_candidates(
  const Network &graph,
  const TGOpath &tg_opath){
  MatchedCandidatePath matched_candidate_path;
  for (int i = 0; i < tg_opath.size(); ++i) {
    const TGNode *a = tg_opath[i];
    if (a==nullptr) {
      matched_candidate_path.push_back(
        {i,-1,-1,-1,0,0,0,0,0,0}
        );
    } else {
      matched_candidate_path.push_back(
        {i,
         a->c->edge->id,
         network.get_node_id(a->c->edge->source),
         network.get_node_id(a->c->edge->target),
         a->c->dist,
         a->c->offset,
         a->c->edge->length,
         a->ep,
         a->tp,
         a->sp_dist}
        );
    }
  }
  return matched_candidate_path;
};
