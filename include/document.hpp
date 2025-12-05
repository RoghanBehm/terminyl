#pragma once
#include <string>
#include <variant>
#include <vector>

#include "source.hpp"

class Document {
        
public:

    struct Heading {
        int level = 0;
        std::string text;
        SourceSpan span{};
    };

    struct Paragraph {
        SourceSpan span{};
        std::string text;
    };

    using Block = std::variant<Heading, Paragraph>;

    const std::vector<Block>& blocks() const { return blocks_; }
    static Document parse(std::istream& in);

private:
    std::vector<Block> blocks_;
};


