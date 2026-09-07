from enum import Enum


class BusType(Enum):
    """Enumeration of all supported bus types"""

    onetoM = "onetoM"
    NtoM = "NtoM"
    outstanding = "outstanding"

    @classmethod
    def parse(cls, value: str):
        """Parse user-facing bus type names, keeping the legacy onetoM spelling."""
        if value == "1toN":
            return cls.onetoM
        return cls(value)
