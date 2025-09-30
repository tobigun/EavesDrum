# the relative_links plugin only works for linked .md files with frontmatter section.
# As we cannot add front-matter to our .md's without github rendering it as a table,
# we have to replace all links to .md with .html manually here.
Jekyll::Hooks.register :pages, :pre_render do |page|
  page.content = page.content.gsub(/\(([^)]+\.md)\)/) do |match|
    "(#{$1.sub(/\.md$/, '.html')})"
  end
end
