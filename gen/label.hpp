#ifndef GEN_LABEL_HPP
#define GEN_LABEL_HPP

#include "gen.hpp"

namespace gen {

Label getLabel(const char *name = "");

void defineLabel(Label label);

} // namespace gen

#endif // GEN_LABEL_HPP
