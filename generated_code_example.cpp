#include <iostream>
#include <string>
using namespace std;
char buffer[1024]; int pos = 0; char ch;
char GetNext(){ return buffer[pos++]; }
bool is_digit(char ch){ return isdigit(ch); }
bool is_letter(char ch){ return isalpha(ch); }
void GetToken_ID(){
  int state = 1; bool done = false; ch = GetNext();
  while (!done) {
    switch (state) {
      case 0:
      { bool moved = false;
        if (is_digit(ch) || is_letter(ch)) { state = 0; ch = GetNext(); moved = true; }
        if (!moved) done = true;
        break; }
      case 1:
      { bool moved = false;
        if (is_letter(ch)) { state = 0; ch = GetNext(); moved = true; }
        if (!moved) done = true;
        break; }
      default: done = true; break;
    }
  }
  if (state == 0) cout << "ID"; else cout << "Error!";
}
int main(){
  string s; getline(cin, s);
  if (s.size() > 1023) { cerr << "Input too long"; return 1; }
  pos = 0; for (size_t i = 0; i < s.size(); ++i) buffer[i] = s[i]; buffer[s.size()] = '\0';
  GetToken_ID();
  return 0;
}