Dim SpVoice, Speech
Set SpVoice = CreateObject("SAPI.SpVoice")
Speech = WScript.Arguments(0) ' Get the text from the command line

SpVoice.Speak Speech

Set SpVoice = Nothing
Set Speech = Nothing
WScript.Quit
