#include "bloom_filter.h"
#include <assert.h>
#include <stdio.h>
#include <cmath>
#include <algorithm>
#include <iostream>

void testConstructor() {
  int d = 10;
  int k = 2;
  // other behaviors to test: expect error if k < 1
  // expect error if k > n
  // expect error if d <= 0
  InvBloom* ibf = new InvBloom(d, k); // alpha defaults to 1.5
  uint32_t expected_size = ceil(d*1.5);
//  fprintf(stdout, "Expected size: %d, Actual: %lu\n", expected_size, ibf->table.size());
  assert(ibf->table.size() == expected_size);
  assert(ibf->n == expected_size);
  assert(ibf->k == k); 
  fprintf(stdout, "Passed testConstructor\n");
  }

void testEncodeHash() {
  int d = 10;
  int k = 3;
  InvBloom* ibf = new InvBloom(d, k); // alpha defaults to 1.5
  int idxs[k];
  uint64_t item = 6458;

//  fprintf(stdout, "calling encodeHash\n");
  ibf->encodeHash(item, idxs);
//  fprintf(stdout, "encodeHash completed\n");

  // Ensure that returned indices are valid (within the bounds
  // of ibf->table and all distinct).
  // Ideally, values should appear random; not sure how to test
  // for that.
  std::sort(idxs, idxs + k);
  assert(idxs[0] >= 0 && idxs[0] < ibf->n); 
  for (int i = 1; i < k; i++) {
    assert(idxs[i] >= 0 && idxs[i] < ibf->n);
    assert(idxs[i] != idxs[i - 1]);
  }
  fprintf(stdout, "Passed testEncodeHash\n");    
}

void testEncodeDecode() {
  int d = 10;
  int k = 3;
  InvBloom* ibf = new InvBloom(d, k); // alpha defaults to 1.5
  std::vector<uint64_t> items = {5, 10, 15};
/*
  int a = 5;
  int b = 10;
  int c = a ^ b;
  std::bitset<64> x(a);
  std::cout << "5: " << x << "\n";
  std::bitset<64> y(b);
  std::cout << "10: " << y << "\n";
  std::bitset<64> z(c);
  std::cout << "5^10 (15): " << z << "\n"; 
  std::cout << "z as number: " << c << "\n";
*/
//  fprintf(stdout, "filter size: %d\n", ibf->n);
  ibf->encode(items);
//  fprintf(stdout, "Finished encode\n");
  std::vector<uint64_t> decoded_items;
  std::vector<uint64_t> expect_empty;
  ibf->decode(&decoded_items, &expect_empty);
/*  fprintf(stdout, "Finished decode\n");
  fprintf(stdout, "decoded_items size: %lu\n", decoded_items.size());
  for (uint64_t i : decoded_items) {
    std::cout << i << ",";
  }
  std::cout << "\n";
  fprintf(stdout, "expect_empty size: %lu\n", expect_empty.size());
  for (uint64_t i : expect_empty) {
    std::cout << i << ",";
  }
*/
  assert(decoded_items.size() == items.size());
  assert(expect_empty.size() == 0);
  std::sort(decoded_items.begin(), decoded_items.end());
  assert(items == decoded_items);
  fprintf(stdout, "Passed testEncodeDecode\n");
}

void testContains() {
  // since there may be false negatives, can't check for 
  // non-membership.
  // Should I have some kind of statistical check for false neg?
  int d = 10;
  int k = 3;
  InvBloom* ibf = new InvBloom(d, k); // alpha defaults to 1.5
  std::vector<uint64_t> items = {5, 10, 15};
  ibf->encode(items);
  for (uint64_t i : items) {
    assert(ibf->contains(i));
  }
  std::vector<uint64_t> random_values = {3, 85, 24, 12, 37};
  int trueCount = 0;
  for (uint64_t r : random_values) {
    if(ibf->contains(r)) { trueCount++; }
  }
  assert(trueCount < random_values.size());
/*  int idxs[3];
  ibf->encodeHash(3, idxs);
  for (int i : idxs) {
    std::cout << i << ",";
  }
  std::cout << "\n";
  fprintf(stdout, "contains 7: %d\n", ibf->contains(3));
*/
  fprintf(stdout, "passed testContains (false +'s out of 5: %d)\n", trueCount);
}

void testSubtract() {
  int d = 10;
  int k = 3;
  InvBloom* ibf1 = new InvBloom(d, k); // alpha defaults to 1.5
  std::vector<uint64_t> s1 = {54, 99, 51, 95, 35, 86, 73, \
                              41, 3, 33, 61, 19, 87, 93, 83};
  ibf1->encode(s1);
  InvBloom* ibf2 = new InvBloom(d, k);
  std::vector<uint64_t> s2 = {54, 99, 12, 95, 35, 4, 73, \
                              41, 21, 33, 61, 19, 6, 93};
  ibf2->encode(s2);

  // s1 - s2
  std::vector<uint64_t> mB_expected = {51, 86, 3, 87, 83};
  // s2 - s1
  std::vector<uint64_t> mA_expected = {12, 4, 21, 6};

  InvBloom* subtracted = new InvBloom(d, k);
  std::vector<uint64_t> mB_actual;
  std::vector<uint64_t> mA_actual;
  ibf1->subtract(*ibf2, subtracted);
  bool success = subtracted->decode(&mB_actual, &mA_actual);
  assert(success == true);
/*  fprintf(stdout, "mB_actual: {");
  for (uint64_t si : mB_actual) {
    std::cout << si << ",";
  }
  std::cout << "}\n";

  fprintf(stdout, "mA_actual: {");
  for (uint64_t si : mA_actual) {
    std::cout << si << ",";
  }
  std::cout << "}\n";
*/

  // Compare sorted vectors to ensure that actual matches
  // expected result.
  std::sort(mB_actual.begin(), mB_actual.end());
  std::sort(mA_actual.begin(), mA_actual.end());
  std::sort(mB_expected.begin(), mB_expected.end());
  std::sort(mA_expected.begin(), mA_expected.end());
  assert(mB_actual == mB_expected);
  assert(mA_actual == mA_expected);
  fprintf(stdout, "passed testSubtract\n");
}

int main() {
  // Things I haven't tested: # elements >> size of filter
  //                          other edge cases
  testConstructor();
  testEncodeHash();
  testEncodeDecode();
  testContains();
  testSubtract();
}
