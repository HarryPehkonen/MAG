#include "input_handler.h"
#include "readline_input_handler.h"
#include "simple_input_handler.h"
#include <iostream>

namespace mag {

std::unique_ptr<InputHandler> create_input_handler() {
#ifdef HAS_READLINE
    std::cout << "MAG using readline for enhanced CLI experience\n";
    return std::make_unique<ReadlineInputHandler>();
#else
    std::cout << "MAG using simple input (readline not available)\n";
    return std::make_unique<SimpleInputHandler>();
#endif
}

} // namespace mag