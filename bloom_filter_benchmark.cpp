#include <chrono>
#include <cmath>
#include <fstream>
#include <string>
#include <iostream>
#include <random>

#include "bloom_filter.h"

struct ExperimentResult {
  int totalCorrect;
  std::vector<std::chrono::duration<double, std::milli>> ts_encode;
  std::vector<std::chrono::duration<double, std::milli>> ts_subtract;
  std::vector<std::chrono::duration<double, std::milli>> ts_decode;
  std::vector<bool> correct;
};

void printVec(const std::vector<uint64_t> v) {
  std::cout << "{";
  for (int i = 0; i < v.size(); i++) {
    std::cout << v[i] << ", ";
  }
  std::cout << "}\n";
}

void generateVectorPair(int size, int d, 
                        std::vector<uint64_t> &u,
                        std::vector<uint64_t> &v) {
  // Knuth's algorithm for selection sampling.
  std::vector<uint64_t> elements;
  int N = 100000;
  int n = 2*size;
  int nextNum = 0;
  std::mt19937 rng(std::random_device{}());
  // Initialize
  int t = 0, m = 0;
  while (elements.size() < n) {
    // Generate U
    double U = std::generate_canonical<double, 64>(rng);
    // Test; if >= threshold, skip, else, select.
    if (float(N-t)*U < (n-m)) {
      m++;
      elements.push_back(nextNum);
    }
    t++;
    nextNum++;
  }

  // Take first v.size() elements for v.
  for (int i = 0; i < size; i++) {
    v.push_back(elements[i]);
  }

  // Generate u
  // Pick random number from 0 to d/2 (this will be number of elts
  //   not copied from v)
  std::random_device random_device;
  std::mt19937 random_engine(random_device());
  std::uniform_int_distribution<int> diffDistr(0, d);
  int numRandom = diffDistr(random_engine);
  std::cout << "num random: " << numRandom << "\n";

  // Compute u size (this implicitly deletes some elements so the
  //   two vectors have different lengths)
  int uSize = v.size() - (int) floor(float(d - numRandom)/2.0); 
  std::cout << "v size: " << v.size() << ", v' size: " << uSize << "\n";
  
  // Add random elements to u
  int idx = v.size();
  for (int i = 0; i < numRandom; i++) {
    u.push_back(elements[idx]);
    idx++;
  } 

  // Fill remaining with elements of v
  int vIdx = 0;
  for (int i = numRandom; i < uSize; i++) {
    u.push_back(v[vIdx]);
    vIdx++;
  }

  // Shuffle u
  std::random_shuffle(u.begin(), u.end());
} 

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
/*  std::cout << "v: {";
  for (int i = 0; i < size; i++) {
    std::cout << v[i] << ", ";
  } 
  std::cout << "}\n";
*/
  return v;
}

// Generate a vector u that differs from v by no more than d elements
// Note: this function ensures |u - v| <= d but does not ensure that
//   |v - u| <= d.
std::vector<uint64_t> generatePartnerVector(std::vector<uint64_t> v, int d) {
  std::vector<uint64_t> u;

  // Pick random number from 0 to d/2
  std::random_device random_device;
  std::mt19937 random_engine(random_device());
  std::uniform_int_distribution<int> diffDistr(0, d);
  int numRandom = diffDistr(random_engine);
  std::cout << "num random: " << numRandom << "\n";

  // Compute u size
  int uSize = v.size() - (int) floor(float(d - numRandom)/2.0); 
  std::cout << "v size: " << v.size() << ", v' size: " << uSize << "\n";
  
  // Add random elements to u
  std::random_device random_device_elts;
  std::mt19937 random_engine_elts(random_device_elts());
  std::uniform_int_distribution<int> distribution_1_10k(1, 10000);

  for (int i = 0; i < numRandom; i++) {
    u.push_back(distribution_1_10k(random_engine_elts));
  } 

  // Fill remaining with elements of v
  int vIdx = 0;
  for (int i = numRandom; i < uSize; i++) {
    u.push_back(v[vIdx]);
    vIdx++;
  }

/*  std::cout << "vec pair: ";
  printVec(u);
  std::cout << "vec: ";
  printVec(v);
*/
  // Note: values in u may not be distinct, but probably are; deal with
  // this if it causes a problem.
  return u;
}

bool checkEncode(const std::vector<uint64_t> v_sorted, std::vector<uint64_t> decoded, const std::vector<uint64_t> expect_empty, bool print) {
  bool correct = true;
  if (print) {
    std::cout << ">>>> Checking correctness\n";
  }
  if (decoded.size() != v_sorted.size()) { 
    if (print) { 
      std::cout << "Expected size " << v_sorted.size() << ", got " << decoded.size() << "\n"; 
    }
    correct = false; 
  }
  if (expect_empty.size() != 0) { 
    if (print) { std::cout << "Expected empty but wasn't\n"; }
    correct = false; 
  }
  std::sort(decoded.begin(), decoded.end());
  if (v_sorted != decoded) { 
    if (print) {
      std::cout << "Expected {";
      for (int i = 0; i < v_sorted.size(); i++) { 
        std::cout << v_sorted[i] << ","; 
      }
      std::cout << "}\n     got {";
      for (int i=0; i<decoded.size();i++) {
        std::cout << decoded[i] << ",";
      }
      std::cout << "}\n";
    }
    correct = false; 
  };
  return correct;
}



bool checkSetDifference(const std::vector<uint64_t> u_sorted, 
                        const std::vector<uint64_t> v_sorted, 
                        std::vector<uint64_t> u_minus_v) {
  std::vector<uint64_t> u_minus_v_expected(u_sorted.size());
//  std::cout << "Computing expected set difference between\n"; 
//  printVec(u_sorted);
//  printVec(v_sorted);
  std::vector<uint64_t>::iterator it = std::set_difference(
    u_sorted.begin(), u_sorted.end(),
    v_sorted.begin(), v_sorted.end(),
    u_minus_v_expected.begin());
//  std::cout << "Set difference computed, resizing\n";
  u_minus_v_expected.resize(it - u_minus_v_expected.begin());

//  std::cout << "expected difference: ";
//  printVec(u_minus_v_expected); std::sort(u_minus_v.begin(), u_minus_v.end());
  std::sort(u_minus_v.begin(), u_minus_v.end());
//  std::cout << "got difference: ";
//  printVec(u_minus_v);

  if (u_minus_v.size() != u_minus_v_expected.size()) {
//    std::cout << "Expected difference size " << u_minus_v_expected.size();
//    std::cout << ", got " << u_minus_v.size() << "\n";
    return false;
  }
  for (int i = 0; i < u_minus_v.size(); i++) {
    if (u_minus_v[i] != u_minus_v_expected[i]) {
      return false;
    }
  }
  return true;
} 


bool checkSubtract(const std::vector<uint64_t> u_sorted, 
                   const std::vector<uint64_t> v_sorted, 
                   std::vector<uint64_t> u_minus_v, 
                   std::vector<uint64_t> v_minus_u, 
                   bool print) {
  if (print) {
    std::cout << ">>>> Checking correctness\n";
  } 

  if (print) { std::cout << "Checking u - v\n"; }
  bool umvCorrect = checkSetDifference(u_sorted, v_sorted, u_minus_v);
  if (print) { std::cout << "Checking v - u\n"; }
  bool vmuCorrect = checkSetDifference(v_sorted, u_sorted, v_minus_u); 

  return (umvCorrect && vmuCorrect);
}


void runExperiment(std::vector<uint64_t> u, 
                   std::vector<uint64_t> v,
                   uint32_t iters,  
                   int d, 
                   int k,
                   std::vector<uint64_t> u_sorted,
                   std::vector<uint64_t> v_sorted, 
                   ExperimentResult* res) {
  std::vector<uint64_t> u_minus_v;
  std::vector<uint64_t> v_minus_u;
  for (int i = 0; i < iters; i++) {
//    std::cout << "iter " << i << "\n";
    // Time the encode operation
    auto begin = std::chrono::steady_clock::now();
    InvBloom* first = new InvBloom(d, k);
    first->encode(u);
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> t_encode = end - begin;
//    std::cout << "encoded\n";
    InvBloom* second = new InvBloom(d, k);
    second->encode(v);
    
    // Time the subtract operation
    InvBloom* result = new InvBloom(d, k);
    begin = std::chrono::steady_clock::now();
    first->subtract(*second, result);
    end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> t_subtract = end - begin;

    // Time decode operation
    begin = std::chrono::steady_clock::now();
    result->decode(&u_minus_v, &v_minus_u);
    end = std::chrono::steady_clock::now();
    std::chrono::duration<double, std::milli> t_decode = end - begin;

    // Check correctness of encode->subtract->decode sequence
    bool correct = checkSubtract(u_sorted, v_sorted, u_minus_v, v_minus_u, false);   
    // Update ExperimentResult
    res->correct.push_back(correct);
    if (correct) { res->totalCorrect++; }
    res->ts_encode.push_back(t_encode);
    res->ts_subtract.push_back(t_subtract);
    res->ts_decode.push_back(t_decode);

    // Reset vectors for next iter
    u_minus_v.clear();
    v_minus_u.clear();
  }
}

int getSetDiffSize(std::vector<uint64_t> u_sorted,
                   std::vector<uint64_t> v_sorted) {
  std::vector<uint64_t> u_minus_v_expected(u_sorted.size());
  std::vector<uint64_t>::iterator it_u = std::set_difference(
    u_sorted.begin(), u_sorted.end(),
    v_sorted.begin(), v_sorted.end(),
    u_minus_v_expected.begin());
  u_minus_v_expected.resize(it_u - u_minus_v_expected.begin());
  return u_minus_v_expected.size();
}

bool onlyUnique(std::vector<uint64_t> v_sorted) {
  for (int i = 1; i < v_sorted.size(); i++) {
    if (v_sorted[i] == v_sorted[i - 1]) { return false; }
  }
  return true;
} 

void runBenchmark(uint32_t iters, int k, int dScale, std::string resFile) {
  std::vector<int> sizes = {100, 500, 1000, 5000, 10000};
//  std::vector<int> sizes = {30};
  std::ofstream results;
  results.open(resFile);
  results << "size,distinct,totalCorrect,diffSize1,diffSize2,d,cells";
  for (int i = 0; i < iters; i++) {
    results << ",t_enc" << i << "(ms)";
  }
  for (int i = 0; i < iters; i++) {
    results << ",t_sub" << i << "(ms)";
  }
  for (int i = 0; i < iters; i++) {
    results << ",t_dec" << i << "(ms)";
  }
  results << "\n";
  for (int s : sizes) {
    int d = (int) ceil(0.3*float(s));
//    std::cout << "Size: " << s << ", d: " << d << "\n";
    std::vector<uint64_t> u;
    std::vector<uint64_t> v; 
    generateVectorPair(s, d, u, v);
    auto u_sorted = u;
    auto v_sorted = v;
    std::sort(u_sorted.begin(), u_sorted.end());
    std::sort(v_sorted.begin(), v_sorted.end());
//    printVec(u);
//    printVec(v);
/*    std::vector<uint64_t> u = generateVector(s);
    std::vector<uint64_t> u_sorted = u;
    std::sort(u_sorted.begin(), u_sorted.end());  
    
    std::vector<uint64_t> v = generatePartnerVector(u, d);
    std::vector<uint64_t> v_sorted = v;
    std::sort(v_sorted.begin(), v_sorted.end());  
*/
    ExperimentResult res = { 0, {}, {} };  
    runExperiment(u, v, iters, (int) floor(dScale*d), k, u_sorted, \
                  v_sorted, &res);

    // Write results for this input size
    results << s << "," << (onlyUnique(u_sorted) && onlyUnique(v_sorted)) << "," << res.totalCorrect << ",";
    results << getSetDiffSize(u_sorted, v_sorted) << ",";
    results << getSetDiffSize(v_sorted, u_sorted) << ",";
    results << d << "," << 1.5*dScale*d;
    for (int i = 0; i < iters; i++) {
      results << "," << res.ts_encode[i].count();
    }
    for (int i = 0; i < iters; i++) {
      results << "," << res.ts_subtract[i].count();
    }
    for (int i = 0; i < iters; i++) {
      results << "," << res.ts_decode[i].count();
    }
    results << "\n";
  }
  results.close();
}

int main() {
  std::string fnameBase = "benchmarkResults/iter_10_k_3_dScale_";
  std::vector<int> dScales = {1, 2, 4, 5, 8, 10, 20};
  for (int dScale : dScales) {
    runBenchmark(10, 3, dScale, fnameBase + std::to_string(dScale) + ".txt");
  }
  std::cout << "Output written to iter_10_k_3_dScale_{1,2,4,5,8,10,20}.txt\n";
} 
