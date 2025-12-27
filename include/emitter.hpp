#pragma once
#include "document.hpp"
#include <string>

struct StyleState {
  bool bold = false;
  bool italic = false;
  bool code = false;

  auto operator<=>(const StyleState &) const = default;

  std::string to_ansi() const {
    if (code)
      return "\x1b[7m";
    if (bold && italic)
      return "\x1b[1;3m";
    if (bold)
      return "\x1b[1m";
    if (italic)
      return "\x1b[3m";
    return "\x1b[0m";
  }
};
struct Style {
  std::size_t width = 80;
  std::size_t paragraph_indent = 0;
};

struct Run {
  std::string text;
  StyleState style;
  bool glue_left;
};

class Emitter {
public:
  explicit Emitter(Style s = {});
  const Style &getStyle() const { return style_; }
  void render(std::ostream &out, const Document &doc) const;
  std::string render_to_string(const Document &doc) const;

private:
  std::string box_heading(std::string_view s, int level,
                          std::size_t pad = 1) const;
  Style style_;
  void wrap_paragraph(std::ostream &out,
                      const std::vector<Document::InlinePtr> &inlines,
                      std::size_t width, std::size_t indent = 0) const;
  void skip_whitespace(std::size_t &i, std::string_view text) const;
  void skip_non_whitespace(std::size_t &i, std::string_view text) const;
  void flatten_runs(const std::vector<Document::InlinePtr> &inlines,
                    StyleState current_style, std::vector<Run> &out) const;
  bool is_punctuation(std::string_view s) const;
};