
desc = "basic test of the tag_escape() filter"

filedata = """
{$
    locals { text: "here is some <b>text<div>and more</div></b>text <i>fin</i> & done" }
$}
${text|tag_escape}
${text|tag_escape|tag_escape}
${text|tag_escape|tag_escape|tag_escape}
"""

conv = "here is some <b>text&lt;div>and more&lt;/div></b>text <i>fin</i> &amp; done"

outcome = "\n".join ([ conv ] * 3)
