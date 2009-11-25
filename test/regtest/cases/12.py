
desc = "regtest to test escaping of %% signs within strings (17.py in OKWS 2.1)"

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
