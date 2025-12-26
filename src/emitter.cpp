#include "emitter.hpp"
#include <iostream>
#include <sstream>
#include <variant>

Emitter::Emitter(Style s) : style_(std::move(s)) {}

void Emitter::render(std::ostream &out, const Document &doc) const {
  for (const auto &blk : doc.blocks()) {
    std::visit(
        [&](const auto &b) {
          using T = std::remove_cvref_t<decltype(b)>;

          if constexpr (std::is_same_v<T, Document::Heading>) {
            out << box_heading(b.text) << "\n";
          } else if constexpr (std::is_same_v<T, Document::Paragraph>) {
            wrap_paragraph(out, b.inlines, style_.width, style_.paragraph_indent);
            out << "\n";
          }
        },
        blk);
  }
}

std::string Emitter::render_to_string(const Document &doc) const {
  std::ostringstream out;
  render(out, doc);
  return out.str();
}

std::string Emitter::box_heading(std::string_view s, std::size_t pad) {
  const std::size_t w = s.size();
  const std::size_t inner = w + 2 * pad;

  std::string out;
  out.reserve((inner + 4) * 3);

  out += '+';
  out += std::string(inner + 2, '-');
  out += "+\n";

  out += '|';
  out += std::string(pad + 1, ' ');
  out += std::string(s);
  out += std::string(pad + 1, ' ');
  out += "|\n";

  out += '+';
  out += std::string(inner + 2, '-');
  out += "+\n";

  return out;
}

void Emitter::skip_whitespace(std::size_t &i, std::string_view text) const {
  while (i < text.size() && std::isspace(static_cast<unsigned char>(text[i])))
    ++i;
}

void Emitter::skip_non_whitespace(std::size_t &i, std::string_view text) const {
  while (i < text.size() && !std::isspace(static_cast<unsigned char>(text[i])))
    ++i;
}

void Emitter::flatten_runs(const std::vector<Document::InlinePtr>& inlines,
                           bool emph,
                           std::vector<Run>& out) const {
    for (auto const& p : inlines) {
        std::visit([&](auto const& node) {
            using T = std::remove_cvref_t<decltype(node)>;
            if constexpr (std::is_same_v<T, Document::Inline::Text>) {
                out.push_back(Run{ node.text, emph });
            } else if constexpr (std::is_same_v<T, Document::Inline::Emph>) {
                flatten_runs(node.children, true, out);
            }
        }, p->node);
    }
}

void Emitter::wrap_paragraph(std::ostream &out,
                             const std::vector<Document::InlinePtr> &inlines,
                             std::size_t width, std::size_t indent) const {
  std::size_t line_len = 0;

  // add n many blank chars, where n is the indent var
  auto write_indent = [&]() {
    out << std::string(indent, ' ');
    line_len = indent;
  };

  write_indent();

  std::vector<Run> runs;
  flatten_runs(inlines, false, runs);

  auto ansi_set_emph = [&](bool on) { out << (on ? "\x1b[1m" : "\x1b[0m"); };

  bool emph_on = false;

  for (auto const &r : runs) {
    std::string_view s = r.text;
    std::size_t i = 0;

    while (i < s.size()) {

      // skip whitespace
      while (i < s.size() && std::isspace((unsigned char)s[i]))
        ++i;
      if (i >= s.size())
        break;

      std::size_t start = i;
      while (i < s.size() && !std::isspace((unsigned char)s[i]))
        ++i;
      std::string_view word = s.substr(start, i - start);

      // wrapping (visible chars only)
      auto word_len = word.size();

      if (line_len == indent) {
        if (emph_on != r.emph) {
          ansi_set_emph(r.emph);
          emph_on = r.emph;
        }
        out << word;
        line_len += word_len;
      } else if (line_len + 1 + word_len <= width) {
        out << ' ';
        line_len += 1;
        if (emph_on != r.emph) {
          ansi_set_emph(r.emph);
          emph_on = r.emph;
        }
        out << word;
        line_len += word_len;
      } else {
        out << '\n';
        write_indent();
        if (emph_on != r.emph) {
          ansi_set_emph(r.emph);
          emph_on = r.emph;
        }
        out << word;
        line_len += word_len;
      }
    }
  }

  if (emph_on)
    ansi_set_emph(false); // reset
  out << '\n';
}