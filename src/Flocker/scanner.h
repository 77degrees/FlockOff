#ifndef SCANNER_H_
#define SCANNER_H_

class SCANNER
{
public:
  SCANNER() {}
  ~SCANNER() {}

  bool begin();
  void survey(uint32_t interval, const char* fname, bool doJson, const char* notes);

private:
  
};

#endif //SCANNER_H_
