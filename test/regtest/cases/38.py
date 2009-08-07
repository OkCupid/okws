
desc = "basic test of the tag_strip() filter"

filedata = """
{$
    setl { text: "here is some <b>text<div>and more</div></b>text <i>fin</i> & done" }
$}
${text|tag_strip}
${text|tag_strip|tag_strip}
${text|tag_strip|tag_strip|tag_strip}
"""

conv = "here is some <b>text&lt;div>and more&lt;/div></b>text <i>fin</i> &amp; done"

outcome = "\n".join ([ conv ] * 3)
