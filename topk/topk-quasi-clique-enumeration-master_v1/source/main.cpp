/*
	@author Seyed-Vahid Sanei-Mehri
	Email contact: vas@iastate.edu
*/

#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <clocale>
#include <chrono>
#include <cstdlib>
#include "load_graph.h"
#include "quickM.h"
#include "list_top_k.h"

void print_quasi_cliques(std::vector< std::pair< int, std::set<int> > > quasi_cliques, load_graph& G) {
	freopen(G.output_address, "w", stdout);
	int cnt = 1;
	for (auto pair : quasi_cliques) {
		auto quasi_clique = pair.second;
		printf("%d. size = %d,\t", cnt++, (int)quasi_clique.size());
		bool flg = true;
		printf("Vertex set = {");
		for (auto node : quasi_clique) {
			if (flg == false) printf(", ");
			flg = false;
			printf("%d", G.map_idx_to_vertex[node]);
		}
		printf("}\n");
	}
}

void print_quasi_cliques_sizes(std::vector< std::pair< int, std::set<int> > > quasi_cliques, load_graph& G) {
	freopen(G.output_address, "w", stdout);
	for (auto pair : quasi_cliques) {
		printf("%d ", (int)pair.second.size());
	}
	printf("\n");
}

void print_max_quasi_clique_summary(const quickM& solver, load_graph& G) {
	freopen(G.output_address, "w", stdout);
	setlocale(LC_NUMERIC, "C");
	int max_size = solver.get_max_clique_size();
	if (max_size <= 0)
		return;
	printf("%d %.6f", max_size, solver.get_max_clique_time());
	const std::vector<int>& vertices = solver.get_max_clique_vertices();
	for (size_t i = 0; i < vertices.size(); i++) {
		auto it = G.map_idx_to_vertex.find(vertices[i]);
		if (it != G.map_idx_to_vertex.end())
			printf(" %d", it->second);
		else
			printf(" %d", vertices[i]);
	}
	printf("\n");
}

int main(int argc, char* argv[]) {
	
	std::ios::sync_with_stdio(false);
	
	if(argc > 1 && std::string(argv[1]) == "exact") {
		load_graph* graph = new load_graph();
		graph->start_exact();
		// quickm extracts quasi cliques with the parameter gamma
		quickM* quickm = new quickM(*graph, graph->gamma);
		quickm->reset_max_clique_tracking();
		quickm->extract_all_quasi_cliques();
		auto all_gamma_quasi_cliques = quickm->get_all_quasi_cliques();
		list_top_k* top_k = new list_top_k();
		auto top_k_gamma_quasi_cliques = top_k->extract_top_k(graph->k, all_gamma_quasi_cliques);
		print_max_quasi_clique_summary(*quickm, *graph);
		return 0;
	}

	if(argc > 1 && argc != 8 && argc != 9) {
		std::cerr << "Usage: " << argv[0] << " <input_file> <output_file> <gamma> <gamma_prime> <k> <k_prime> <min_size> [max_runtime_seconds]" << std::endl;
		return 1;
	}

	double max_runtime_seconds = 0.0;
	if(argc == 9) {
		max_runtime_seconds = atof(argv[8]);
		if(max_runtime_seconds <= 0.0) {
			std::cerr << "max_runtime_seconds must be positive" << std::endl;
			std::cerr << "Usage: " << argv[0] << " <input_file> <output_file> <gamma> <gamma_prime> <k> <k_prime> <min_size> [max_runtime_seconds]" << std::endl;
			return 1;
		}
	}
	
	// run heuristic algorithm
	load_graph* graph = new load_graph();
	if(argc == 8 || argc == 9)
		graph->start(argv[1], argv[2], atof(argv[3]), atof(argv[4]), atoi(argv[5]), atoi(argv[6]), atoi(argv[7]));
	else
		graph->start();

	std::chrono::steady_clock::time_point runtime_start = std::chrono::steady_clock::now();

	// quickm extracts quasi cliques with the parameter gamma_prime
	quickM* quickm = new quickM(*graph, graph->gamma_prime);
	quickm->set_runtime_limit(runtime_start, max_runtime_seconds);
	quickm->extract_all_quasi_cliques();
	auto all_gamma_prime_quasi_cliques = quickm->get_all_quasi_cliques();
	list_top_k* top_k_prime = new list_top_k();
	auto top_k_gamma_prime_quasi_cliques = top_k_prime->extract_top_k(graph->k_prime, all_gamma_prime_quasi_cliques);

	// quickm_2 uses gamma
	quickM* quickm_2 = new quickM(*graph, graph->gamma);
	quickm_2->reset_max_clique_tracking();
	quickm_2->set_runtime_limit(runtime_start, max_runtime_seconds);
	for (auto pair : top_k_gamma_prime_quasi_cliques) {
		if (quickm_2->has_timed_out())
			break;
		auto quasi_clique = std::vector<int> (pair.second.begin(), pair.second.end());
		quickm_2->expand(quasi_clique);
	}
	auto gamma_quasi_cliques = quickm_2->get_all_quasi_cliques();
	
	list_top_k* top_k = new list_top_k();
	auto top_k_gamma_quasi_cliques = top_k->extract_top_k(graph->k, gamma_quasi_cliques);

	if (max_runtime_seconds > 0.0 && (quickm->has_timed_out() || quickm_2->has_timed_out()))
		std::cerr << " Runtime limit reached after " << max_runtime_seconds << " seconds" << std::endl;

	if (max_runtime_seconds > 0.0 && (quickm->has_timed_out() || quickm_2->has_timed_out()) && quickm_2->get_max_clique_size() <= 0)
		print_max_quasi_clique_summary(*quickm, *graph);
	else
		print_max_quasi_clique_summary(*quickm_2, *graph);
	return 0;
}
