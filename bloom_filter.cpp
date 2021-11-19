#include "bloom_filter.h"
#include <cmath>
#include <functional>
#include <set>
#include <stdio.h>
#include <string> 
#include <iostream>

// Constants
const IbfCell kDefaultCell = {0, 0, 0};

// Methods
InvBloom::InvBloom(uint32_t d, uint32_t k, float alpha, float query_threshold) {
  this->n = (uint32_t) ceil(d*alpha);
  this->k = k;
  this->query_threshold = query_threshold;
  this->table.resize(n, kDefaultCell);  
} 

InvBloom::~InvBloom() {
}
  
// Encode the set as an invertible bloom filter and store the
// result in this.
void InvBloom::encode(const std::vector<uint64_t> &set) {
  int idxs[this->k]; // init to 0 
  for (uint64_t s_i : set) {
    encodeHash(s_i, idxs);
/*    std::cout << "idxs for " << s_i << ": ";
    for (int t = 0; t < this->k; t++) { 
      std::cout << idxs[t] << ",";
    }
*/
//    std::cout << "\n";
    for (int j : idxs) {
      // bounds check
      if (j < 0 || j >= this->n) { continue; } // TODO raise error 
      // todo check that indices are distinct
      // using bitwise xor; TODO correct?
/*      if (j==14) {
        std::cout << "idSum: " << this->table[j].idSum << "\n";
        std::cout << "var idSum: " << idSum << "\n";
        std::cout << "s_i: " << s_i << "\n";
        std::cout << "xor: " << (idSum ^ s_i) << "\n";
      }
*/
      this->table[j].idSum ^= s_i;
      this->table[j].hashSum ^= checksumHash(s_i);
      this->table[j].count++; 
    }
  }
//  fprintf(stdout, "Encode result:\n%s", this->to_string().c_str());
}

// Subtract IBF "other" from this IBF and store the result in
// result.
// TODO for subtract and subtract_cell, should I just overwrite
// this with result instead of making a new IBF?
void InvBloom::subtract(const InvBloom &other, InvBloom *result) {
  if (other.k != this->k || result->k != this->k) {
    // TODO raise error
  }
  if (other.n != this->n || result->n != this->n) {
    // TODO raise error
  }
  for (int i = 0; i < this->n; i++) {
    subtractCell(i, other.table[i], &result->table[i]);
  }
}

// Decode this IBF (results only make sense if this IBF was
// derived by subtracting two IBFs A and B, or this IBF = A-B). 
// missingB: list of elements that A contains but B doesn't.
// missingA: list of elements that B contains but A doesn't.
// Returns true if decoded successfully, false otherwise.
bool InvBloom::decode(std::vector<uint64_t> *missingB, std::vector<uint64_t> *missingA) {
  // Find pure elements
  std::vector<int> pure_idxs;
  
  for (int i = 0; i < this->n; i++) {
    if (isPure(i)) { 
      pure_idxs.push_back(i); 
//      std::cout << i << "\n";
    } 
  }

  int distinct_idxs[this->k]; // holds distinct idxs for given elt

  while (pure_idxs.size() > 0) {
    int i = pure_idxs.back();
    pure_idxs.pop_back();
    // Confirm elt at i is still pure.
    if (!isPure(i)) { continue; }
    // Add to appropriate set difference list
//    std::cout << "pure: " << i << "\n";
    int c = this->table[i].count;
    uint64_t ids = this->table[i].idSum;
//    std::cout << "c: " << c << " idSum: " << ids << "\n";
    if (c > 0) { 
//      fprintf(stdout, "Adding to missingB\n");
      missingB->push_back(ids);
//      fprintf(stdout, "Added to missingB\n");
    } else {
      missingA->push_back(ids);
    }
//    fprintf(stdout, "Added to vector\n");
    // Remove this element from IBF and update
    // any new pure cells.
    encodeHash(ids, distinct_idxs);
/*    for (int k : distinct_idxs) {
      std::cout << k;
    }
    std::cout << "\n";
*/
    for (int j : distinct_idxs) {
      this->table[j].count -= c;
      this->table[j].hashSum = this->table[j].hashSum ^ checksumHash(ids);
      this->table[j].idSum = this->table[j].idSum ^ ids;
      if (isPure(j)) { pure_idxs.push_back(j); }
    }
  }

  // No pure indices remain; check that all fields in IBF are 0.
  for (int i = 0; i < this->n; i++) {
    if (this->table[i].count != 0) { return false; }
    if (this->table[i].hashSum != 0) { return false; }
    if (this->table[i].idSum != 0) { return false; }
  }
  return true;
} 

bool InvBloom::isPure(int idx) {
  int c = this->table[idx].count;
  if (c != 1 and c != -1) { return false; }
  uint32_t hs = this->table[idx].hashSum;
  uint64_t ids = this->table[idx].idSum;
  if (hs == checksumHash(ids)) { return true; }
  return false; 
}

// Only valid on output of encode. Cannot be used on output
// of subtract (results are meaningless).
bool InvBloom::contains(const uint64_t elt) {
  int distinct_idxs[this->k];   
  encodeHash(elt, distinct_idxs);
//  fprintf(stdout, "table:\n%s", this->to_string().c_str());
  for (int idx : distinct_idxs) {
//    fprintf(stdout, "i: %d\n", idx);
    if (this->table[idx].count < this->query_threshold) { 
      return false;
    }
  }
  return true;
}

// Subtract IBF cell "other" from this IBF and store the result
// in result.
void InvBloom::subtractCell(const uint32_t idx, const IbfCell &other, IbfCell *result) {
  result->idSum = this->table[idx].idSum ^ other.idSum;
  result->hashSum = this->table[idx].hashSum ^ other.hashSum;
  result->count = this->table[idx].count - other.count;
}

// Return checksum hash; hash function should be distinct from
// that used for encodeHash.
uint32_t InvBloom::checksumHash(const uint64_t &elt) {
  std::hash<std::string> hash_elt;
  // Hacky way of making this hash distinct from idx hash:
  // "salt" with fixed string.
  return hash_elt(std::to_string(elt)+"checksum");
}

// Populate "indices" (size of array is k) with computed indices
// resulting from executing hash function.
void InvBloom::encodeHash(const uint64_t &elt, int indices[]) {
/*  if (sizeof(indices)/sizeof(indices[0]) != this->k) {
    // TODO raise exception
  }
*/
  std::set<int> idxs;
  std::hash<std::string> hash_elt;
  // currently doing a hacky thing to get hash values to be different;
  // should probably be a cryptographic hash function
  std::size_t prev_hash = hash_elt(std::to_string(elt));
  while (idxs.size() < this->k) {
//    fprintf(stdout, "k: %d, # idxs: %d, prev_hash: %d, idx: %d\n", this->k, idxs.size(), prev_hash, prev_hash % this->n);
    idxs.insert(prev_hash % this->n);
    prev_hash = hash_elt(std::to_string(prev_hash));
  }
  int count = 0;
  for (int i : idxs) {
    indices[count] = i;
    count++;
  }
}

