#include <string>
#include "document.hpp"
class Emitter {

public:
void render(std::ostream& out, const Document& doc) const;
std::string render_to_string(const Document& doc) const;

private:
static std::string box_heading(std::string_view text, std::size_t pad = 1);

};