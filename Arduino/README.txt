RDM (TX_RETRY_DELAY_MULTIPLIER) values are the retry delay multipliers used when
acks are wanted.  To help prevent repeated collisions, use 1, a prime number
(2, 3, 5, 7, 11, 13), or 15 (the maximum) for the multiplier.  "--" indicates
no acks wanted.


Arduino Sketch Name        Used for Widget(s)         RDM
-------------------------- ------------------------- ----
Widget_Bells               Bells                      15

Widget_BoogieBoard         Boogie Board                3

Widget_ContortOMatic       Contort-O-Matic            15

Widget_Esplora             Esplora development board  (2)

Widget_Eye                 Eye                        15

Widget_FourPlay            FourPlay                   15
                           FourPlay-4-2                7
                           FourPlay-4-3                4
                           TriObelisk                  6
                           Spinnah                    11

Widget_MSGEQ7              Mike #1                     7
                           Mike #2                     6

Widget_MPU6050             Baton1                     13
                           Baton2                     11
                           BoogieBoard                --
                           Flower1                     9
                           Flower2                     8
                           Flower3                     5
                           Flower4                     7
                           Flower5                    11
                           Flower6                    13
                           Flower7                    15
                           Rainstick                  --
                           Rattle1                     7
                           Rattle2                     5

Widget_Plunger             Pump                       13

Widget_SchroedersPlaything Schroeder's Plaything       2

Widget_StressTest          widget radio tester        (3)

