
desc = "test of comments"

filedata="""
[[[Comment in HTML mode should be stripped out entirely]]]
{$
    // comments in C++ mode that /* are not used and stripped */
    /*
     *  C-style comments also work
     *  fine if that sort of things floats your baot.
     */
     locals { /*comment here */ x : "[[[[comment]]inX[[out]]inX]]",
             y /* comment here */ : "[[[trips are gone]]]inY" } 
$}

[[ in1 in2 in3 ${x} ${y} [[ out1 out2 [[ out3 ]] out4 ]] in4 [in5] 
\[[escape\]] in6 in7 [[out5 ${out6} out7 out8]] in8]
[ [in9 in10 in11 ]] 

[[[[out out]] in20 in21 [[out out out]] in22 in23]]
[[[out10 out11 out12 out13]]]
[[[out1 out2 out3 out4 out5]]
[[[out1 out2 out3 out4 out5]]]]
[[ in30 in31 in32 [[[out1 out2 out3]]] in33 in34 ]]
[[[ out10 out11 [[ out12 ]]]
[[[[out12 out13]] in41 in42 in43]]

${x}
${y}
${x/*stuff not needed */}
"""

outcome="""
in1 in2 in3 inXinX inY in4 [in5] 
[[escape]] in6 in7 in8] 
[ [in9 in10 in11
in20 in21 in22 in23 ]
in30 in31 in32 ] in33 in34
in41 in42 in43
inXinX
inY
inXinX
"""
