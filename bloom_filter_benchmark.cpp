#include <chrono>
#include <cmath>
#include <fstream>
#include <string>
#include <random>

#include "bloom_filter.h"

struct ExperimentResult {
  int totalCorrect;
  std::vector<std::chrono::duration<double, std::milli>> intervals;
  std::vector<bool> correct;
};

std::vector<uint64_t> generateVector(int size) {
  // Seed and initialize an object that randomly draws from a uniform
  // distribution of values from 1 to 10000.
  std::random_device random_device;
  std::mt19937 random_engine(random_device());
  std::uniform_int_distribution<int> distribution_1_100(1, 10000);

  std::vector<uint64_t> v;
  for (int i = 0; i < size; i++) {
    v.push_back(distribution_1_100(random_engine));
  }  
  return v;
}

void runExperiment(std::vector<uint64_t> v, uint32_t iters, int d, int k,
                   std::vector<uint64_t> v_sorted, ExperimentResult* res) {
  std::vector<uint64_t> decoded_items;
  std::vector<uint64_t> expect_empty;
  for (int i = 0; i < iters; i++) {
    // Time the encode operation
    auto begin = std::chrono::steady_clock::now();
    InvBloom* b = new InvBloom(d, k);
    b->encode(v);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> interval = end - begin;

    // Check that encode was correct by decoding (implicitly also
    // checks correctness of decode).
    bool correct = true;
    b->decode(&decoded_items, &expect_empty);
    if (decoded_items.size() != v_sorted.size()) { correct = false; }
    if (expect_empty.size() != 0) { correct = false; }
    std::sort(decoded_items.begin(), decoded_items.end());
    if (v_sorted != decoded_items) { correct = false; };

    // Update ExperimentResult
    res->correct.push_back(correct);
    if (correct) { res->totalCorrect++; }
    res->intervals.push_back(interval);

    // Reset vectors for next iter
    decoded_items.clear();
    expect_empty.clear();
  }
}

void runBenchmark(uint32_t iters, int k, std::string resFile) {
//  std::vector<int> sizes = {100, 500, 1000, 5000, 10000};
  std::vector<int> sizes = {10, 20};
  std::ofstream results;
  results.open(resFile);
  results << "size,totalCorrect";
  for (int i = 0; i < iters; i++) {
    results << ",t" << i;
  }
  results << "\n";
  for (int s : sizes) {
    std::vector<uint64_t> v = generateVector(s);
    std::vector<uint64_t> v_sorted = v;
    std::sort(v_sorted.begin(), v_sorted.end());  
    ExperimentResult res = { 0, {}, {} };  
    runExperiment(v, iters, (int)ceil(0.1*(float)s), k, v_sorted, &res);
    results << s << "," << res.totalCorrect;
    for (int i = 0; i < iters; i++) {
      results << "," << res.intervals[i].count();
    }
    results << "\n";
  }
  results.close();
}

int main() {
  runBenchmark(10, 3, "benchmarkResults/iter_10_k_3.txt");
} 
