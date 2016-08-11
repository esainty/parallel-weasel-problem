#include <string>
#include <cstdlib>
#include <iostream>
#include <cassert>
#include <algorithm>
#include <vector>
#include <cmath>
#include <mpi.h>
#include <tuple>
#include <typeinfo>
std::string allowed_chars = " ABCDEFGHIJKLMNOPQRSTUVWXYZ";
// class selection contains the fitness function, encapsulates the
// target string and allows access to it's length. The class is only
// there for access control, therefore everything is static. The
// string target isn't defined in the function because that way the
// length couldn't be accessed outside.
class selection {
	public:
		// this function returns 0 for the destination string, and a
		// negative fitness for a non-matching string. The fitness is
		// calculated as the negated sum of the circular distances of the
		// string letters with the destination letters.
		static double fitness(std::string candidate) {
			assert(target.length() == candidate.length());
			double fitness_so_far = 0.0;
			double correctcount = 0.0;
			for (int i = 0; i < target.length(); ++i) {
				if (target[i] == candidate[i]) {
					correctcount++;
				}
				// int target_pos = allowed_chars.find(target[i]);
				// int candidate_pos = allowed_chars.find(candidate[i]);
				// int diff = std::abs(target_pos - candidate_pos);
				// fitness_so_far -= std::min(diff, int(allowed_chars.length()) - diff);
			}
			fitness_so_far = correctcount / target.length();
			return fitness_so_far;
		}
		// get the target string length
		static int target_length() { 
			return target.length(); 
		}
		static std::string get_target() {
			return target;
		}
		static void set_target(std::string newtarget) {
			target = newtarget;
		}
	private:
		static std::string target;
};
std::string selection::target = "METHINKS IT IS LIKE A WEASEL";
int rank;
// helper function: cyclically move a character through allowed_chars
void move_char(char& c, int distance) {
	while (distance < 0)
	distance += allowed_chars.length();
	int char_pos = allowed_chars.find(c);
	c = allowed_chars[(char_pos + distance) % allowed_chars.length()];
}
// mutate the string by moving the characters by a small random
// distance with the given probability
std::string mutate(std::string parent, double mutation_rate) {
	// if (rank == 1) {
	// 	std::cout << "Parent: " << parent << "\n";
	// }
	for (int i = 0; i < parent.length(); ++i) {
		if (std::rand()/(RAND_MAX + 1.0) < mutation_rate) {
			int distance = std::rand() % 3 + 1;
			if (std::rand()%2 == 0) {
				move_char(parent[i], distance);
			} else {
				move_char(parent[i], -distance);
			}
		}
	}
	// if (rank == 1) {
	// 	std::cout << "Child: " << parent << "\n";
	// }
	return parent;
}
// helper function: tell if the first argument is less fit than the
// second
bool less_fit(std::string const& s1, std::string const& s2) {
	return selection::fitness(s1) < selection::fitness(s2);
}

std::vector<std::tuple<std::string, double>> arrange(std::vector<std::string> population) {
	std::vector<std::tuple<std::string, double>> sorted_population;
	std::sort(population.begin(), population.end(), less_fit);
	for (int i = 0; i < population.size(); i++) {
		sorted_population.push_back(std::make_tuple(population[i], selection::fitness(population[i])));
		// std::cout << "String: " << std::get<0>(sorted_population[i]) << " Double: " << std::get<1>(sorted_population[i]) <<  "\n";
	}
	return sorted_population;
}

std::string selectParent(std::vector<std::tuple<std::string, double>> population, int type) {
	std::string selection;
	switch(type) {
		//Steady State Strategy.
		//All in top 20% have an equal chance of being chosen.	
		default :
			int indexA = std::floor(std::rand()/(RAND_MAX + 1.0) * (population.size() * 0.2)) + std::floor(population.size() * 0.8);
			selection = std::get<0>(population[indexA]);
	}
	return selection;
}

std::string crossover(std::tuple<std::string, std::string> parents, int type) {
	std::string offspring;
	switch(type) {
		case 1 : {
			//One-Point Strategy.
			//Left part from one parent, right part from the other. 
			//Crossover point chosen randomly.
			int mid = std::floor(std::rand()/(RAND_MAX + 1.0) * std::get<0>(parents).length());
			for (int i = 0; i < mid; i++) {
				offspring.push_back(std::get<0>(parents)[i]);
			}
			for (int i = mid; i < std::get<0>(parents).length(); i++) {
				offspring.push_back(std::get<1>(parents)[i]);
			}
			break;
		}
		case 2 : {
			//Local Fitness Strategy.
			//If a character matches the target string, it is used in the child, otherwise chosen randomly.
			for (int i = 0; i < std::get<0>(parents).length(); i++) {
				if (std::get<0>(parents)[i] == selection::get_target()[i]) {
					offspring.push_back(std::get<0>(parents)[i]);
				} else if (std::get<1>(parents)[i] == selection::get_target()[i]) {
					offspring.push_back(std::get<1>(parents)[i]);
				} else {
					if (std::rand()%2 == 0) {
						offspring.push_back(std::get<0>(parents)[i]);
					} else {
						offspring.push_back(std::get<1>(parents)[i]);
					}
				}
			}
			break;
		}
		default : {
			//Uniform Strategy.
			//Each character of child is chosen randomly from one of the parents.	
			for (int i = 0; i < std::get<0>(parents).length(); i++) {
				if (std::rand()%2 == 0) {
					offspring.push_back(std::get<0>(parents)[i]);
				} else {
					offspring.push_back(std::get<1>(parents)[i]);
				}
			}
		}
	}
	return offspring;
}

void sendString(std::string expression, int destination) {
	int length = expression.length() + 1;
	//printf("Length: %d, Rank: %d\n", length, rank);
	char transmittableExpression[length];
	strcpy(transmittableExpression, expression.c_str());
	MPI_Send(&transmittableExpression, length, MPI_CHAR, destination, 97, MPI_COMM_WORLD);
}

std::string recvString(int source) {
	std::string expression;
	int length;
	char transmittableExpression[length];
	MPI_Status status;
	MPI_Probe(source, 97, MPI_COMM_WORLD, &status);
	//printf("Probed\n");
	MPI_Get_count(&status, MPI_CHAR, &length);
	//printf("Received Length: %d, Rank: %d, Source: %d\n", length, rank, source);
	MPI_Recv(&transmittableExpression, length, MPI_CHAR, source, 97, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	//printf("Post-Received Length: %d, Rank: %d, Source: %d\n", length, rank, source);
	expression = std::string(transmittableExpression);
	return expression;
}

int main(int argc, char *argv[]) {
	int selector = 0;
	std::srand(time(0));
	int world;
	MPI_Init(&argc, &argv); 
	MPI_Comm_size(MPI_COMM_WORLD, &world); 
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	if (rank == 0) {
		std::string best;
		double best_fitness = 0;
		int generation_counter = 1;
		int child_counter = 0;
		typedef std::vector<std::string> childvec;
		childvec population;
		printf("Please choose selection strategy:\n0. Steady State (default)\n");
		int selectionIN;
		std::cin >> selectionIN;
		printf("Please choose crossover strategy:\n0. Uniform (default)\n1. One-Point\n2. Local-Fitness\n");
		int crossoverIN;
		std::cin >> crossoverIN;
		printf("Please choose mutation rate (0-1)\n");
		double mutation_rate;
		std::cin >> mutation_rate;
		printf("Please input generation population size\n");
		int P = 100;
		std::cin >> P;
		std::cin.ignore();
		printf("Please input target string\n");
		std::string target_string;
		std::getline(std::cin, target_string);
		std::cout << target_string << "\n";
		selection::set_target(target_string);
		population.reserve(P);
		for (int i = 0; i < P; i++) {
			std::string parent;
			for (int i = 0; i < selection::target_length(); ++i) {
				parent += allowed_chars[std::rand() % allowed_chars.length()];
			}	
			population.push_back(parent);
		}
		for (int i = 1; i < world; i++) {
			MPI_Send(&crossoverIN, 1, MPI_INT, i, 90, MPI_COMM_WORLD);
			MPI_Send(&mutation_rate, 1, MPI_DOUBLE, i, 91, MPI_COMM_WORLD);
		}
		for(int fitness = 0; fitness < 1; fitness = best_fitness) {
			std::vector<std::tuple<std::string, double>> sorted_population = arrange(population);
			childvec generation;
			for (int i = 1; i < world; i++) {
				std::string selection = selectParent(sorted_population, selectionIN);
				sendString(selection, i);
				selection = selectParent(sorted_population, selectionIN);
				sendString(selection, i);
				child_counter++;
			}
			for (int i = 0; i < P; ++i) {
				MPI_Status status;
				MPI_Probe(MPI_ANY_SOURCE, 97, MPI_COMM_WORLD, &status);
				int slave = status.MPI_SOURCE;
				std::string child = recvString(slave);
				generation.push_back(child);
				//If there are still more within the population size to request.
				if (i < P - (world - 1)) {
					std::string selection = selectParent(sorted_population, selectionIN);
					sendString(selection, slave);
					selection = selectParent(sorted_population, selectionIN);
					sendString(selection, slave);
					child_counter++;
				}
			}
			population = generation;
			best = *std::max_element(population.begin(), population.end(), less_fit);
			best_fitness = selection::fitness(best);
			std::cout << "Closest String: " << best << ", Fitness: " << best_fitness << "\n";
			generation_counter++;
		}
		int flag = 1;
		for (int i = 1; i < world; i++) {
			MPI_Send(&flag, 1, MPI_INT, i, 98, MPI_COMM_WORLD);
		}
		std::cout << "Final String: " << best << "\n";
		std::cout << "Number of generations: " << generation_counter << "\n";
		std::cout << "Total number of children: " << child_counter << "\n";
	} else if (rank != 0) {
		int flag = 0;
		int probe = 0;
		int crossoverIN;
		double mutationIN;
		MPI_Recv(&crossoverIN, 1, MPI_INT, 0, 90, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		MPI_Recv(&mutationIN, 1, MPI_DOUBLE, 0, 91, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
		while (!flag) {
			MPI_Iprobe(0, 98, MPI_COMM_WORLD, &flag, MPI_STATUS_IGNORE);
			MPI_Iprobe(0, 97, MPI_COMM_WORLD, &probe, MPI_STATUS_IGNORE);
			if (probe) {
				std::string parentA = recvString(0);
				std::string parentB = recvString(0);
				std::tuple<std::string, std::string> selection = std::make_tuple(parentA, parentB);
				std::string child = crossover(selection, crossoverIN);
				child = mutate(child, mutationIN);
				sendString(child, 0);
			}
		}
	}
	MPI_Finalize();
}