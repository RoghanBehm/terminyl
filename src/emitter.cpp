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
            out << box_heading(b.text, b.level) << "\n";
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

std::string Emitter::box_heading(std::string_view s, int level, std::size_t pad) const {
  const std::size_t w = s.size();
  const std::size_t inner = w + 2 * pad;
  
  // Box style chosen based on heading level
  struct BoxChars {
    const char* top_left;
    const char* top_right;
    const char* bottom_left;
    const char* bottom_right;
    const char* horizontal;
    const char* vertical;
  };
  
  BoxChars chars;
  switch (level) {
    case 1:  // h1 ('=')
      chars = {"╔", "╗", "╚", "╝", "═", "║"};
      break;
    case 2:  // h2 ('==')
      chars = {"┏", "┓", "┗", "┛", "━", "┃"};
      break;
    case 3:  // h3 ('===')
      chars = {"┌", "┐", "└", "┘", "─", "│"};
      break;
    default:  // >= h4 ('====')
      chars = {"╭", "╮", "╰", "╯", "─", "│"};
      break;
  }
  
  std::string horizontal_line;
  for (std::size_t i = 0; i < inner + 2; ++i) {
    horizontal_line += chars.horizontal;
  }
  
  std::string out;
  out += chars.top_left + horizontal_line + chars.top_right + "\n";
  out += std::string(chars.vertical) + std::string(pad + 1, ' ') + 
         std::string(s) + std::string(pad + 1, ' ') + chars.vertical + "\n";
  out += chars.bottom_left + horizontal_line + chars.bottom_right + "\n";
  
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

  for (size_t run_idx = 0; run_idx < runs.size(); ++run_idx) {
    auto const &r = runs[run_idx];
    std::string_view s = r.text;
    std::size_t i = 0;
    
    bool first_word_in_run = true;

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
        if (first_word_in_run && current_state != r.style && current_state != StyleState{}) {
          out << "\x1b[0m"; 
          out << ' ';
          line_len += 1;
        } else {
          out << ' ';
          line_len += 1;
        }
      }

      // Apply style if changed
      if (current_state != r.style) {
        out << r.style.to_ansi();
        current_state = r.style;
      }

      out << word;
      line_len += word_len;
      first_word_in_run = false;
    }
  }

  // Reset styles
  if (current_state != StyleState{}) {
    out << "\x1b[0m";
  }
  out << '\n';
}