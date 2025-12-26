#include "emitter.hpp"
#include <iostream>
#include <sstream>
#include <variant>

Emitter::Emitter(Style s) : style_(std::move(s)) {}

void Emitter::render(std::ostream &out, const Document &doc) const {
  for (const auto &blk : doc.blocks()) {
    // Type-based dispatch
    std::visit(
        [&](const auto &b) {
          using T = std::remove_cvref_t<decltype(b)>;

          if constexpr (std::is_same_v<T, Document::Heading>) {
            out << box_heading(b.text) << "\n";
          } else if constexpr (std::is_same_v<T, Document::Paragraph>) {
            wrap_paragraph(out, b.inlines, style_.width,
                           style_.paragraph_indent);
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
  
  std::string horizontal_line;
  horizontal_line.reserve(inner + 2);
  for (std::size_t i = 0; i < inner + 2; ++i) {
    horizontal_line += "─";
  }
  
  std::string out;
  out.reserve((inner + 4) * 3 * 3);
  
  out += "┌" + horizontal_line + "┐\n";
  out += "│" + std::string(pad + 1, ' ') + std::string(s) + std::string(pad + 1, ' ') + "│\n";
  out += "└" + horizontal_line + "┘\n";
  
  return out;
}

void Emitter::flatten_runs(const std::vector<Document::InlinePtr> &inlines,
                           StyleState current_style,
                           std::vector<Run> &out) const {
  for (auto const &p : inlines) {
    std::visit(
        [&](auto const &node) {
          using T = std::remove_cvref_t<decltype(node)>;
          if constexpr (std::is_same_v<T, Document::Inline::Text>) {
            out.push_back(Run{node.text, current_style});
          } else if constexpr (std::is_same_v<T, Document::Inline::Bold>) {
            StyleState child_style = current_style;
            child_style.bold = true;
            flatten_runs(node.children, child_style, out);
          } else if constexpr (std::is_same_v<T, Document::Inline::Italic>) {
            StyleState child_style = current_style;
            child_style.italic = true;
            flatten_runs(node.children, child_style, out);
          } else if constexpr (std::is_same_v<T, Document::Inline::Code>) {
            StyleState child_style = current_style;
            child_style.code = true;
            out.push_back(Run{node.text, child_style});
          }
        },
        p->node);
  }
}

void Emitter::wrap_paragraph(std::ostream &out,
                             const std::vector<Document::InlinePtr> &inlines,
                             std::size_t width, std::size_t indent) const {
  std::size_t line_len = 0;
  auto write_indent = [&]() {
    out << std::string(indent, ' ');
    line_len = indent;
  };

  write_indent();

  std::vector<Run> runs;
  flatten_runs(inlines, StyleState{}, runs);

  StyleState current_state;

  for (auto const &r : runs) {
    std::string_view s = r.text;
    std::size_t i = 0;

    while (i < s.size()) {
      // skip whitespace
      while (i < s.size() && std::isspace((unsigned char)s[i]))
        ++i;
      if (i >= s.size())
        break;

      // extract word
      std::size_t start = i;
      while (i < s.size() && !std::isspace((unsigned char)s[i]))
        ++i;
      std::string_view word = s.substr(start, i - start);
      auto word_len = word.size();

      // Check if wrap necessary
      bool needs_wrap =
          (line_len != indent) && (line_len + 1 + word_len > width);

      if (needs_wrap) {
        out << '\n';
        write_indent();
      } else if (line_len != indent) {
        if (current_state != StyleState{}) {
          out << "\x1b[0m"; // Reset to default
          current_state = StyleState{};
        }
        out << ' ';
        line_len += 1;
      }

      // Apply style if changed
      if (current_state != r.style) {
        out << r.style.to_ansi();
        current_state = r.style;
      }

      out << word;
      line_len += word_len;
    }
  }

  // Reset styles at end
  if (current_state != StyleState{}) {
    out << "\x1b[0m";
  }
  out << '\n';
}