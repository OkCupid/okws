desc = "test hidden escape"

filedata = """
{$
   setl { text : 'foo <bar> "hi" & \\'bye\\'' }
$}
${text|hidden}
"""

outcome = "foo &lt;bar&gt; &quot;hi&quot; &amp; &#039;bye&#039;"

