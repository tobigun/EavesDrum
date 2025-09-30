module Jekyll
  class TocGenerator < Generator
    safe true
    priority :low

    def generate(site)
      page = site.pages.find { |p| p.name == "README.md" }
      return unless page

      headings = page.content.scan(/^(#+)\s+(.*)/)
      site.config['readme_toc'] = headings.map do |hashes, text|
        {
          'text' => text.strip,
          'depth' => hashes.length
        }
      end
    end
  end
end
