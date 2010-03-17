desc = "check WSS-safe zones (pre and script)"

filedata ="""
blah
   foo
       bar
<pre>x    y  11
z
n
</pre>
{% for (i, range (2)) {{<script>1 2  3    4</script>}} %}
"""

outcome_exact = \
""" blah foo bar <pre>x    y  11
z
n
</pre> <script>1 2  3    4</script><script>1 2  3    4</script> """
