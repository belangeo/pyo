"""
EventParser first draft (very very alpha stage!).

Score format:

Class_name => starttime duration *args **kwargs

Class_name {class reference}: The name of the class which should play the event,
starttime {float}: Start time of the event, in seconds. 
duration {float}: Start time of the event, in seconds.
*args {list of floats}: user-defined arguments (optional).
**kwargs {dictionary}: user-defined keyword arguments (optional).
 
"""
# Notes:
# We should be able to define an event using multiple lines.
# Actually, the parser uses splitlines() to separate events.
# How to automatically use globals() from the __main__ module, I don't
# want to ask the user to pass globals() to have access to instrument classes.

class EventParser:
    def __init__(self, server, score="", globals=None):
        self.server = server
        self.globals = globals
        self._instruments = {}
        self.parse(score)

    def extractKwArgs(self, args):
        kwargs = ""
        if "{" in args:
            p1 = args.find("{")
            args, kwargs = args[:p1], args[p1:]
        if kwargs:
            kwargs = eval(kwargs)
        return args, kwargs

    def parse(self, score):
        for line in score.splitlines():
            if line.strip() == "":
                continue
            instr, args = line.split("=>")
            instr = instr.strip()
            args, kwargs = self.extractKwArgs(args)
            args = [float(x) for x in args.split()] # should also eval variable
                                                    # names, not just numbers
            if instr not in self._instruments:
                self._instruments[instr] = [{"args": args, "kwargs": kwargs}]
            else:
                self._instruments[instr].append({"args": args, "kwargs": kwargs})

    def play(self):
        self._playing = []
        for instr in self._instruments:
            for event in self._instruments[instr]:
                self.server.setGlobalDel(event["args"][0])
                self.server.setGlobalDur(event["args"][1] + 0.2)
                if not event["kwargs"]:
                    self._playing.append(self.globals[instr](*event["args"][1:]))
                else:
                    self._playing.append(self.globals[instr](*event["args"][1:], 
                                                             **event["kwargs"]))
        self.server.setGlobalDel(0)
        self.server.setGlobalDur(0)
