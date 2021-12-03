#include <stdint.h>
#include <vector>
#include <string>

struct IbfCell {
  int count;
  uint64_t idSum;
  uint32_t hashSum; // is 4 bytes enough?
};

class InvBloom {
  public:
    uint32_t n; // number of cells; set to d*alpha
    uint32_t k; // number of hash functions
    float query_threshold; 
    std::vector<IbfCell> table; // array of cells

    // Constructor: takes desired number of cells and # hash fns.
    // Precondition: k < d*alpha
    InvBloom(uint32_t d, uint32_t k, float alpha=1.5, float query_threshold=1); 
 
    // Class destructor
    ~InvBloom();
 
    // Encode the set as an invertible bloom filter and store the
    // result in this.
    void encode(const std::vector<uint64_t> &set);

    // Subtract IBF "other" from this IBF and store the result in
    // result.
    // TODO for subtract and subtract_cell, should I just overwrite
    // this with result instead of making a new IBF?
    bool subtract(const InvBloom &other, InvBloom *result);

    // Decode this IBF. 
    // missingB: list of elements that A contains but B doesn't.
    // missingA: list of elements that B contains but A doesn't.
    // If the IBF is NOT a the result of a subtraction, then
    // it should be the result of encode. In that case:
    //   missingB: initially encoded set of elements
    //   missingA: empty.
    // Returns true if decoded successfully, false otherwise.
    bool decode(std::vector<uint64_t> *missingB, std::vector<uint64_t> *missingA); 

    // Returns true if this IBF contains elt, false otherwise.
    bool contains(const uint64_t elt);

    // public only for testing purposes
    // Populate "indices" (size of array is k) with computed indices
    // resulting from executing hash function.
    void encodeHash(const uint64_t &elt, int indices[]);

    std::string to_string() {
      std::string cells = "";
      for (int i = 0; i < n; i++) {
        std::string cell = std::to_string(i) + " | count: " + \
          std::to_string(this->table[i].count) + " idSum: " + \
          std::to_string(this->table[i].idSum) + " hashSum: " + \
          std::to_string(this->table[i].hashSum) + "\n";
        cells += cell;
      }
      return cells;
    }

  private: 
    // Subtract IBF cell "other" from this IBF and store the result
    // in result.
    void subtractCell(const uint32_t idx, const IbfCell &other, IbfCell *result);

    // Return checksum hash; hash function should be distinct from
    // that used for encodeHash.
    uint32_t checksumHash(const uint64_t &elt);
  
    // Return true if this index is pure; false otherwise
    // pure: count = 1 or -1
    // checksumHash(idSum) = hashSum
    bool isPure(int idx); 
};

/*
T* p = null;
T a();  
p = &a;
a.x = 1; // equivalent to p->x
a.y = 2; // equiv. to p->y
p = new T();
p = new T(); // this is a memory leak
*/

