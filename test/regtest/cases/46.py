
desc = "regtest to test espacing of %% signs within strings"

filedata = """

{$ locals { x : "ok to %pass through",
         y : "ok to \\%pass through",
         z : "ok to \\%{pass} through",
         v : "ok to \\\\pass through"
       } $}

${x}
${y}
${z}
${v}
"""

outcome = '\n'.join (["ok to %pass through",
                      "ok to %pass through",
                      "ok to %{pass} through",
                      "ok to \\pass through" ])
