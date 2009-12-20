desc = "test hidden escape"

filedata = """
{$
   locals { text : 'foo <bar> "hi" & \\'bye\\'' }
$}
${text|hidden_escape}
"""

outcome = "foo &lt;bar&gt; &quot;hi&quot; &amp; &#039;bye&#039;"

