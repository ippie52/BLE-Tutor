#!/usr/bin/python3
# ------------------------------------------------------------------------------
"""@package colour_printer.py

Provides a very simple class that allows coloured and named print messages to
help disambiguate between messages from different components.
"""
# ------------------------------------------------------------------------------
#                  Kris Dunning ippie52@gmail.com 2020.
# ------------------------------------------------------------------------------

class ColourPrinter(object):
    """
    Provides the print function with coloured text and a name identifier
    """

    # Foreground colours
    NORMAL = '\u001b[0m'
    BLACK = '\u001b[30m'
    RED = '\u001b[31m'
    GREEN = '\u001b[32m'
    GOLD = '\u001b[33m'
    DARK_BLUE = '\u001b[34m'
    PURPLE = '\u001b[35m'
    TEAL = '\u001b[36m'
    GREY = '\u001b[37m'
    SILVER = '\u001b[90m'
    BRIGHT_RED = '\u001b[91m'
    BRIGHT_GREEN = '\u001b[92m'
    YELLOW = '\u001b[93m'
    BLUE = '\u001b[94m'
    MAGENTA = '\u001b[95m'
    CYAN = '\u001b[96m'
    WHITE = '\u001b[97m'

    # Background colours
    BG_BLACK = '\u001b[40m'
    BG_RED = '\u001b[41m'
    BG_GREEN = '\u001b[42m'
    BG_GOLD = '\u001b[43m'
    BG_DARK_BLUE = '\u001b[44m'
    BG_PURPLE = '\u001b[45m'
    BG_TEAL = '\u001b[46m'
    BG_GREY = '\u001b[47m'
    BG_SILVER = '\u001b[100m'
    BG_BRIGHT_RED = '\u001b[101m'
    BG_BRIGHT_GREEN = '\u001b[102m'
    BG_YELLOW = '\u001b[103m'
    BG_BLUE = '\u001b[104m'
    BG_MAGENTA = '\u001b[105m'
    BG_CYAN = '\u001b[106m'
    BG_WHITE = '\u001b[107m'

    def __init__(self, colour: str, name: str = None) -> None:
        """
        Constructs the object

        Args:
            colour: The colour code to apply to the messages
            name: The name of this object
        """
        self.colour_name = name
        self.colour = colour

    def print(self, *message):
        """
        Prints the message to screen with colour and (if provided) name

        Args:
            message: Collection of message components
        """
        prefix = f'{self.colour}[{self.colour_name}]:' \
            if self.colour_name is not None else self.colour
        print(prefix, *message, ColourPrinter.NORMAL)

