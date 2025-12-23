"""Support for modelling text property behaviour."""
from __future__ import annotations

import re
from dataclasses import dataclass, field
from itertools import count, zip_longest
from pathlib import Path
from typing import ClassVar, Iterator, NamedTuple, Any
from weakref import proxy

import vim

# Bindings for Vim functions.
assert_equal = vim.Function('assert_equal')
assert_report = vim.Function('assert_report')
prop_add = vim.Function('prop_add')
prop_list = vim.Function('prop_list')
prop_type_add = vim.Function('prop_type_add')
prop_remove = vim.Function('prop_remove')
prop_type_delete = vim.Function('prop_type_delete')
prop_type_get = vim.Function('prop_type_get')

prop_add_kwargs = (
    'type', 'length', 'end_lnum', 'end_col', 'bufnr', 'id', 'text',
    'text_align', 'text_padding_left', 'text_wrap')

type_to_highlight: dict[str, str] = {
    '1': 'DiffAdd',
    '2': 'DiffChange',
    '3': 'DiffDelete',
    '4': 'DiffText',
    '5': 'IncSearch',
    '6': 'ErrorMsg',
    '7': 'WarningMsg',
    '8': 'Directory',
    '9': 'ColorColumn',
}


def create_property(
        buffer: BufferState,
        prop_type: PropertyType,
        pos: tuple[int, int],
        flags: PropFlags,
    ) -> Property:
    """Create a property."""
    ln, cn = pos
    kwargs = {
        'buffer': buffer,
        'prop_type': prop_type,
        'lnum': ln,
        'col': cn,
    }
    if flags.text is not None:
         kwargs['text'] = flags.text
         if cn == 0 or flags.is_floating:
            kwargs['col'] = 0
            kwargs['text_align'] = flags.text_align
            kwargs['text_padding_left'] = flags.text_padding_left
            kwargs['text_wrap'] = flags.text_wrap
            return FloatingText(**kwargs)
         else:
            return VirtualText(**kwargs)
    else:
        kwargs['col'] = cn
        if flags.id is not None:
            kwargs['id'] = flags.id
        return HighlightProperty(**kwargs)


def tidy_dict(d: dict[bytes, Any]) -> dict[str, Any]:
    """Convert bytes to str and sort the keys."""
    new_d = {}
    for key, value in sorted(d.items()):
        if isinstance(key, bytes):
            key = key.decode('utf-8', errors='ignore')
        if isinstance(value, bytes):
            value = value.decode('utf-8', errors='ignore')
        new_d[key] = value
    return new_d


def make_obj_dict(obj: Any, names: tuple[str, ...]) -> dict[str, Any]:
    """Create a dictionary for non-None attributes of an object."""
    d = {}
    for name in names:
        value = getattr(obj, name, None)
        if value is not None:
            d[name] = value
    return d


class VimPos(NamedTuple):
    """A Vim buffer position of line number, column number, both 1-based.

    The column number may be zero to indicate the position before the first
    character.
    """
    lnum: int
    cnum: int

    def __repr__(self) -> str:
        return str(tuple(self))


@dataclass
class PropFlags:
    """Parsed flags used to define a property/property type."""
    id: str | None = None
    text: str | None = None
    start_incl: int | None = None
    end_incl: int | None = None
    priority: int | None = None
    text_align: str | None = None
    text_padding_left: int | None = None
    text_wrap: str | None = None

    @classmethod
    def from_flags_spec(cls, flags_spec: list[str]) -> PropFlags:
        """Create using a list of text specifications."""
        kwargs = {}
        for ent in flags_spec:
            if not ent.strip():
                continue
            a, eq, b = ent.partition('=')
            if eq:
                kwargs[a] = eval(b)
            else:
                kwargs[ent] = 1
        return cls(**kwargs)

    @property
    def is_floating(self) -> bool:
        """Test if a floating text flag is set."""
        flags = self.text_align, self.text_padding_left, self.text_wrap
        return flags != (None, None, None)


@dataclass
class PropertyType:
    """A model of a property type, as for ``prop_type_add()``."""
    name: str
    start_incl: int | None = None
    end_incl: int | None = None
    priority: int | None = None

    def create(self):
        """Create as a global property, removing any previous version."""
        prop_type_delete(self.name)
        kwargs = {
            'highlight': type_to_highlight[self.name],
        }
        kwargs.update(
            make_obj_dict(self, ('start_incl', 'end_incl', 'priority')))
        prop_type_add(self.name, kwargs)

    def build_spec_flags(self, names: tuple[str] | None = None) -> list[str]:
        """Build a list of requested flags."""
        flags = []
        names = names or ('start_incl', 'end_incl', 'priority')
        for name in names:
            value = getattr(self, name)
            if value is not None:
                flags.append(f' {name}={value!r}')
        return flags


@dataclass
class Property:
    """Base for the model of a property, as for ``prop_add()``."""
    buffer: BufferState
    prop_type: PropertyType
    lnum: int
    col: int
    is_virtual: ClassVar[bool] = False
    is_floating: ClassVar[bool] = False

    def __post_init__(self):
        self.buffer = proxy(self.buffer)

    @property
    def can_extend_right(self) -> bool:
        """True when insertion after extends the property's length'"""
        return False

    @property
    def type_name(self) -> str:
        """The name of the type for this property."""
        return self.prop_type.name

    @property
    def spec(self) -> str:
        """The specification as a string."""
        repr = [
            self.type_name if self.contains_pos(pos) else ' '
            for pos in self.buffer.line_map
        ]
        repr.extend(self.build_spec_flags())
        return ''.join(repr)

    @property
    def start(self) -> VimPos:
        """The start position of this property as a `VimPos`."""
        return VimPos(self.lnum, self.col)

    @property
    def end(self) -> VimPos:
        """The end position of this property as a `VimPos`."""
        return VimPos(self.end_lnum, self.end_col)

    def create(self) -> None:
        """Create this property for the current buffer."""
        kwargs = make_obj_dict(self, prop_add_kwargs)
        kwargs['type'] = self.prop_type.name
        prop_add(self.lnum, self.col, kwargs)

    def prop_list_entry(self, lnum: int) -> dict[str, str | int]:
        """How this property appears in ``prop_list(lnum)`` output."""
        return {
            'type_bufnr': 0,
            'type': self.prop_type.name,
            'col': self.col,
        }

    def contains_pos(self, pos: VimPos) -> bool:
        """Test if a Vim position lies within the property's cell range."""
        return self.start <= pos < self.end

    def is_in_line(self, lnum: int) -> bool:
        """Test if this property is in a given line."""
        return self.start.lnum <= lnum <= self.end.lnum

    def extend(self, pos: tuple[int, int]):
        """Extend the length of this property."""

    def descr(self) -> str:
        """Describe this property."""
        s = [f'{self.__class__.__name__}:']
        for name, value in sorted(self.__dict__.items()):
            if name.startswith('_') or name in ('buffer', 'prop_type'):
                continue
            s.append(f'{name}={value!r}')
        return ' '.join(s)


@dataclass
class HighlightProperty(Property):
    """A property that highlights text.

    This is *not* used for virtual text properties.
    """
    end_lnum: int | None = None
    end_col: int | None = None
    id: int | None = None
    is_virtual: ClassVar[bool] = False

    def __post_init__(self):
        if self.end_lnum is None:
            self.end_lnum = self.lnum
        if self.end_col is None:
            if self.end_lnum > self.lnum:
                self.end_col = 1
            else:
                self.end_col = self.col + 1
        if self.id is None:
            self.id = 42

    def extend(self, pos: tuple[int, int]):
        """Extend the length of this property."""
        self.end_lnum, self.end_col = pos
        self.end_col += 1

    @property
    def can_extend_right(self) -> bool:
        """True when insertion after extends the property's length'"""
        return self.prop_type.end_incl

    def build_spec_flags(self) -> list[str]:
        """Build a list of any flags for this property's specification line."""
        flags = self.prop_type.build_spec_flags()
        if self.id is not None:
            flags.append(f'id={self.id!r}')
        return flags

    def prop_list_entry(self, lnum: int) -> dict[str, str | int]:
        """How this property appears in ``prop_list(lnum)`` output."""
        if not self.is_in_line(lnum):
            return {}
        entry = super().prop_list_entry(lnum)
        entry.update({
            'id': self.id,
            'start': 0 if self.start.lnum < lnum else 1,
            'end': 0 if self.end.lnum > lnum else 1,
        })
        if self.start.lnum < lnum < self.end.lnum:
            entry['length'] = len(self.buffer.lines[lnum - 1]) + 1
            entry['col'] = 1
        elif self.start.lnum < lnum:
            entry['length'] = self.end_col - 1
            entry['col'] = 1
        elif self.end.lnum > lnum:
            entry['length'] = len(self.buffer.lines[lnum - 1]) - self.col + 2
            entry['col'] = self.col
        else:
            entry['length'] = self.end_col - self.col
            entry['col'] = self.col
        return entry

    def descr(self) -> str:
        """Describe this property."""
        s = [f'{self.__class__.__name__}[{self.type_name}]:']
        if self.end_lnum > self.lnum:
            s.append(f'{self.lnum}.{self.col}->{self.end_lnum}.{self.end_col}')
        else:
            s.append(f'{self.lnum}.{self.col}->{self.end_col}')
        return ' '.join(s)


@dataclass
class VirtualText(Property):
    """A property that shows virtual text.

    Instances of this are used for in-line virtual text. Floating virtual text
    properties are modelled using `FloatingText`.
    """
    text: str
    is_virtual: ClassVar[bool] = True
    is_floating: ClassVar[bool] = False

    @property
    def end(self) -> VimPos:
        """The end position of this property as a `VimPos`."""
        return VimPos(self.lnum, self.col + 1)

    def test_delete(self, start: VimPos, end: VimPos) -> bool:
        """Test if a text deletion operation removes this property."""
        return start < self.start <= end
        if end.lnum < self.lnum or start >= self.start:
            return True       # This property is unaffected.

        if start < self.start <= end:
            return False      # This property is removed.

        if start.lnum == end.lnum:
            count = end_col - start.col + 1
            self.col -= count
        else:
            self.col = 1
        return True

    def build_spec_flags(self) -> list[str]:
        """Build a list of any flags for this property's spec line."""
        flags = [f' text={self.text!r}']
        flags.extend(self.prop_type.build_spec_flags(('priority',)))
        return flags

    def prop_list_entry(self, lnum: int) -> dict[str, str | int]:
        """How this property appears in ``prop_list(lnum)`` output."""
        if not self.is_in_line(lnum):
            return {}
        entry = super().prop_list_entry(lnum)
        entry.update({
            'start': 1,
            'end': 1,
            'text': self.text,
        })
        return entry

    def descr(self) -> str:
        """Describe this property."""
        s = [f'{self.__class__.__name__}[{self.type_name}]:']
        s.append(f'{self.lnum}.{self.col}')
        s.append(f'{self.text!r}')
        return ' '.join(s)


@dataclass
class FloatingText(VirtualText):
    """A property that shows virtual text.

    Instances of this are used for in-line virtual text. Floating virtual text
    properties are modelled using `FloatingText`.
    """
    text_align: str | None = None
    text_padding_left: int | None = None
    text_wrap: str | None = None
    is_virtual: ClassVar[bool] = False
    is_floating: ClassVar[bool] = True

    def __post_init__(self):
        super().__post_init__()
        self.col = 0

    @property
    def end(self) -> VimPos:
        """The end position of this property as a `VimPos`."""
        return VimPos(self.lnum, 1)

    def build_spec_flags(self) -> list[str]:
        """Build a list of any flags for this property's spec line."""
        flags = [f' text={self.text!r}']
        for name in ('text_align', 'text_padding_left', 'text_wrap'):
            value = getattr(self, name)
            if value is not None:
                flags.append(f' {name}={value!r}')
        flags.extend(self.prop_type.build_spec_flags(('priority',)))
        return flags

    def prop_list_entry(self, lnum: int) -> dict[str, str | int]:
        """How this property appears in ``prop_list(lnum)`` output."""
        if not self.is_in_line(lnum):
            return {}
        entry = super().prop_list_entry(lnum)
        entry['text_align'] = self.text_align or 'after'
        # TODO: Docs say these should appear, but they do not.
        # entry['text_padding_left'] = self.text_padding_left or '0'
        # entry['text_wrap'] = self.text_wrap or 'truncated'
        return entry

    def descr(self) -> str:
        """Describe this property."""
        s = [f'{self.__class__.__name__}[{self.type_name}]:']
        s.append(f'{self.lnum}')
        s.append(f'{self.text!r}')
        return ' '.join(s)


@dataclass
class BufferState:
    """A model of the state of a buffer, with applied properties."""
    prop_types: dict[str, PropertyType]
    props: list[Property]
    lines: list[str]
    line_map: list[tuple[int, int]]
    set_prop_types: dict[str, PropertyType] = field(default_factory=dict)

    handle_source: ClassVar[Iterator] = count()
    instances: ClassVar[dict[int, BufferState]] = {}

    @classmethod
    def from_spec(cls, spec:list[str]) -> BufferState:
        """Create and add properties from a string specification."""
        spec = [line.decode('utf-8', errors='ignore') for line in spec]
        spec = [line for line in spec if not line.startswith('#')]

        # Build the sequence of lines for the buffer's text and the line map.
        text_spec = spec.pop(0)
        ts_len = len(text_spec)
        lines, line_map = cls.parse_text_line_spec(text_spec)

        # Process specification lines to create properties and types.
        prop_types: dict[str, PropertyType] = {}
        props: list[Property] = []
        buf = cls(prop_types, props, lines, line_map)
        for line in spec:
            props.extend(
                cls.parse_prop_line_spec(
                    buf, line, ts_len, line_map, prop_types))

        return buf

    @property
    def text_spec(self) -> list[str]:
        """The specification of the buffer's text as a string."""
        return '|' + '|'.join(self.lines) + '|'

    @property
    def spec(self) -> list[str]:
        """The specification as a list of strings."""
        spec = [self.text_spec]
        spec.extend(prop.spec for prop in self.props)
        return spec

    def apply(self) -> None:
        """Set up the current buffer to match the modelled state."""
        self.clean_up()
        vim.current.buffer[:] = self.lines
        for prop_type in self.prop_types.values():
            prop_type.create()
            self.set_prop_types[prop_type.name] = prop_type
        for prop in self.props:
            prop.create()

    def delete_property_types(self) -> None:
        """Delete all property types set by this model."""
        for prop_type in self.set_prop_types.values():
            prop_type_delete(prop_type.name)
        self.set_prop_types = {}

    def clean_up(self) -> None:
        """Try to remove all applied property types and properties."""
        type_names = [name for name in self.prop_types if prop_type_get(name)]
        if type_names:
            prop_remove({'types': type_names, 'all': 1})
        for prop_type in self.set_prop_types.values():
            prop_type_delete(prop_type.name)
        self.set_prop_types = {}

    def remove_property_from_line(self, lnum: int, type_name: str) -> None:
        """Remove a property for a given type from a line."""
        pos = VimPos(lnum, 0)
        if (si := self.find_line_map_pos(pos)) < 0:
            return
        pos = VimPos(lnum + 1, 0)
        if (ei := self.find_line_map_pos(pos)) < 0:
            return
        ei += 1

        new_props: list[Property] = []
        prop_iter = iter(self.props)
        for prop in prop_iter:
            # print("A",
            #     prop.type_name, type_name, prop.start, pos, prop.end)
            if prop.type_name == type_name:
                if prop.lnum <= lnum <= prop.end.lnum:
                    break
            new_props.append(prop)
        else:
            prop = None
        if prop:
            spaces = ' ' * (ei - si)
            new_spec = list(prop.spec[:si] + spaces + prop.spec[ei:])
            ts_len = len(self.line_map)
            new_props.extend(
                self.parse_prop_line_spec(
                    self, ''.join(new_spec), ts_len, self.line_map, {}))
        new_props.extend(prop_iter)
        self.props = new_props

    def delete_text(self, lnum: int, col: int, count: int):
        """Delete chars (including newlines) starting at a given position.

        This updates the model only.
        """
        # Get start end and VimPos.
        start = VimPos(lnum, col)
        if (si := self.find_line_map_pos(start)) < 0:
            return
        ei = si + count
        if ei >= len(self.line_map) + 1:
            assert_report(f'BufferState.delete_text: Invalid {count=}')
            return
        end = self.line_map[ei - 1]

        new_text_spec = self.text_spec[:si] + self.text_spec[ei:]
        if len(new_text_spec) < 2:
            new_text_spec = '||'  # Handle degenerate case.
        lines, line_map = self.parse_text_line_spec(new_text_spec)
        ts_len = len(line_map)
        new_props = []
        old_ts_len = len(self.line_map)
        for prop in self.props:
            if prop.is_floating:
                new_props.append(prop)
                continue
            elif prop.is_virtual:
                if start < prop.start <= end:
                    continue  # The property is removed by deletion.
                elif start >= prop.start:
                    new_props.append(prop)
                    continue  # Deletion is at or after virtual text.

            prop_part, flags = prop.spec[:old_ts_len], prop.spec[old_ts_len:]
            new_spec = list(prop_part[:si] + prop_part[ei:])
            ezipped = enumerate(zip(new_spec, line_map))
            for i, (c, (ln, cn)) in ezipped:
                if c != ' ':
                    if cn == 0:
                        new_spec[i] = ' '
                    break
            for i, (c, (ln, cn)) in ezipped:
                if c == ' ':
                    if i > 0 and cn == 1:
                        new_spec[i - 1] = ' '
                    break

            if len(new_spec) < 2:
                new_spec = [' ', ' ']  # Handle degenerate case.
            new_spec[ts_len - 1] = ' '
            new_props.extend(
                self.parse_prop_line_spec(
                    self, ''.join(new_spec) + flags, ts_len, line_map, {}))

        if start.lnum < end.lnum:
            # Floating properties are removed from all but the last of the
            # joined lines.
            to_drop = []
            for i, prop in enumerate(new_props):
                if not prop.is_floating:
                    continue
                if start.lnum <= prop.lnum < end.lnum:
                    to_drop.append(i)
                elif prop.lnum == end.lnum:
                    prop.lnum = start.lnum
                elif prop.lnum > end.lnum:
                    prop.lnum -= (end.lnum - start.lnum)
            while to_drop:
                new_props.pop(to_drop.pop())

        self.props = new_props
        self.lines = lines
        self.line_map = line_map

    def insert_text(self, lnum: int, col: int, text: str):
        """Insert text after at a given position.

        This updates the model only.
        """
        text = text.decode('utf-8', errors='ignore')
        start = VimPos(lnum, col)
        for ci, pos in enumerate(self.line_map):
            if pos == start:
                break
        else:
            assert_report(f'BufferState.insert_text: Invalid {lnum=} {col=}')
            return

        ci += 1
        new_text_spec = self.text_spec[:ci] + text + self.text_spec[ci:]
        lines, line_map = self.parse_text_line_spec(new_text_spec)
        ts_len = len(line_map)
        new_props = []
        prop_chars = '1234567890'
        for prop in self.props:
            left, right = prop.spec[:ci], prop.spec[ci:]
            insert_c = ' '
            left_prop_type = left[-1] if left else ' '
            right_prop_type = right[0] if right else ' '
            if left_prop_type != ' ' and right_prop_type != ' ':
                insert_c = left_prop_type
            elif left_prop_type != ' ' and right_prop_type != ' ':
                insert_c = ' '
            elif right_prop_type == ' ':
                if prop.can_extend_right:
                    insert_c = left_prop_type
            else:
                insert_c = ' '
            new_spec = left + insert_c * len(text) + right
            new_props.extend(
                self.parse_prop_line_spec(
                    self, new_spec, ts_len, line_map, {}))

        self.props = new_props
        self.lines = lines
        self.line_map = line_map

    def check_buffer_contents(self, add_msg: str = '') -> None:
        """Check that the lines and properties are as expected for all lines.

        This checks that the buffer's text exactly matches as well as the
        properties. This check acts, mainly, as a sanity check on the
        correctness of the test code and the BufferState modelling behaviour.

        This method tries to avoid producing too much 'noise' when failures
        occur:

        1. First it checks that the buffer's text content matches. If it does
           not then any difference between the expected and actual line count
           is reported along with the first non-matching line.

        2. If step 1 does not produce any failures then the properties are
           checked. A difference in the property list sizes is reported and
           reporting stops after the first mismatched pair of lines.
        """
        buf = vim.current.buffer
        text_mismatched = False
        msg = ''
        if len(buf) < len(self):
            msg = 'Buffer contains too many lines'
        elif len(buf) > len(self):
            msg = 'Buffer contains too few lines'
        if msg:
            if add_msg:
                msg = f'{add_msg}: {msg}'
            assert_equal(len(buf), len(self), msg)
        max_line_index = min(len(buf), len(self)) - 1
        for i, (expected, actual) in enumerate(zip(self.lines, buf)):
            msg = f' Line {i + 1}'
            if i < max_line_index:
                msg += ' later lines not checked'
            if add_msg:
                msg = f'{add_msg}: {msg}'
            if assert_equal(expected, actual, msg) == 1:
                text_mismatched = True
                break
        if text_mismatched:
            # Since this is commonly caused by bugs during test development,
            # it is more helpful to stop checking at this point.
            return

        properties_mismatched = False
        for lidx in range(len(self.lines)):
            expected_list = [
                tidy_dict(d) for d in self.expected_prop_list(lidx + 1)]
            actual_list = [tidy_dict(d) for d in prop_list(lidx + 1)]
            msg = ''
            if len(actual_list) > len(expected_list):
                msg = f'Line {lidx + 1} contains too many properties'
            elif len(actual_list) < len(expected_list):
                msg = f'Line {lidx + 1} contains too few properties'
            if msg:
                if add_msg:
                    msg = f'{add_msg}: {msg}'
                assert_equal(len(expected_list), len(actual_list), msg)
                properties_mismatched = True

            skipped = ''
            if lidx < max_line_index:
                skipped = ' (later lines not checked)'
            both = zip_longest(expected_list, actual_list)

            # Try to match properties, without caring about the order, it makes
            # test failure easier to understand.
            matched = []
            missing = []
            unexpected = []
            close_match = []
            while expected_list and actual_list:
                expected = expected_list.pop(0)
                for i, actual in enumerate(actual_list):
                    if expected == actual:
                        matched.append((expected, actual))
                        break
                else:
                    for i, actual in enumerate(actual_list):
                        if self.close_prop_dict_match(expected, actual):
                            close_match.append((expected, actual))
                            break
                    else:
                        missing.append(expected)
                        continue
                actual_list.pop(i)

            for expected, actual in close_match:
                msg = f'Line {lidx + 1}{skipped}'
                msg += f' prop type={expected["type"]!r}, similar'
                if add_msg:
                    msg = f'{add_msg}: {msg}'
                assert_equal(expected, actual, msg)
                properties_mismatched = True
            for prop in missing:
                msg = f'Line {lidx + 1}{skipped}'
                msg += f' prop type={prop["type"]!r}, not found!'
                if add_msg:
                    msg = f'{add_msg}: {msg}'
                assert_equal(prop, {}, msg)
                properties_mismatched = True
            for prop in actual_list:
                msg = f'Line {lidx + 1}{skipped}'
                type_name = prop.get('type')
                if type_name:
                    msg += f' prop type={type_name!r}, not expected!'
                else:
                    msg += f' not expected!'
                if add_msg:
                    msg = f'{add_msg}: {msg}'
                assert_equal({}, prop, msg)
                properties_mismatched = True

            if properties_mismatched:
                break

    def close_prop_dict_match(self, expected, actual):
        """Test if expected and actual property dicts almost match."""
        if sorted(expected.keys()) != sorted(actual.keys()):
            return False
        if 'text' in expected:
            if expected['text'] != actual['text']:
                return False
        if 'type' in expected:
            if expected['type'] != actual['type']:
                return False
        n_ok = n_bad = 0
        for key, value in expected.items():
            if actual[key] == value:
                n_ok += 1
            else:
                n_bad += 1
        return n_bad <= 2 and n_ok >= 4

    def expected_prop_list(self, lnum: int) -> list[dict[str, int | str]]:
        """Generate the expected properties, as for ``prop_list(lnum)``."""
        def get_key(el):
            i, a = el
            return a['col'], i

        expected = [prop.prop_list_entry(lnum) for prop in self.props]
        expected = list(enumerate([entry for entry in expected if entry]))
        expected.sort(key=get_key)
        return [el for i, el in expected]

    def expected_prop_for_type(
            self, lnum: int, prop_type: str) -> dict[str, int | str]:
        """Generate the expected property for a given type."""
        for prop in self.expected_prop_list(lnum):
            if prop['type'] == prop_type:
                return prop
        return {}

    @staticmethod
    def parse_text_line_spec(text_spec: str) -> tuple[list[str], list[VimPos]]:
        """Parse the text part of a specification.

        :return:
            A tuple or lines, line_map.
        """
        lines = text_spec[1:-1].split('|')
        line_map: list[VimPos] = []
        for ln, line in enumerate(lines, 1):
            line_map.extend(VimPos(ln, ci) for ci in range(len(line) + 1))
        line_map.append(VimPos(len(lines) + 1, 0))
        return lines, line_map

    @staticmethod
    def parse_prop_line_spec(
            buf: BufferState,
            prop_spec: str,
            ts_len: int,
            line_map: list[VimPos],
            prop_types: dict[str, PropertyType],
        ) -> list[Property]:
        """Parse a property part of a specification.

        As a side effect, entries can be added to the `prop_types` ``dict``.

        :return:
            A list of properties.
        """
        props: list[Property] = []
        prop: Property | None = None
        prop_part, flags_part = prop_spec[:ts_len], prop_spec[ts_len:]
        flags = PropFlags.from_flags_spec(flags_part.split(' '))
        prev_c = ' '
        for ci, c in enumerate(prop_part):
            if c in '1234567890_':
                if c == '_':
                    c = prev_c
                prev_c = c
                prop_type = prop_types.get(c)
                if prop_type is None:
                    prop_type = PropertyType(
                        c, flags.start_incl, flags.end_incl,
                        flags.priority)
                prop_types[c] = prop_type
                if prop is None:
                    prop = create_property(
                        buf, prop_type, line_map[ci], flags)
                    props.append(prop)
                else:
                    prop.extend(line_map[ci])
            else:
                prop = None
        return props

    def find_line_map_pos(self, pos: VimPos) -> int:
        """Find the index of a position in the line map."""
        for i, mpos in enumerate(self.line_map):
            if mpos == pos:
                return i
        assert_report(f'Invalid position {pos}')
        return -1

    def dump(self) -> None:
        """Dump details of this buffer state instance."""
        print("Properties")
        for prop in self.props:
            print(f'  {prop.descr()}')
        print("Specification")
        for line in self.spec:
            print(f'  {line}')

    def __len__(self) -> int:
        return len(self.lines)


def tp_load_buffer_spec(spec: list[string], apply: bool = False) -> int:
    """Load a buffer specification.

    This creates a `BufferState` instance and returns an integer handle that
    can be used to reference it in other functions.
    """
    buf = BufferState.from_spec(spec)
    if apply:
        buf.apply()
    handle = next(BufferState.handle_source)
    BufferState.instances[handle] = buf
    return handle


def tp_apply_buffer(handle: int) -> None:
    """Make the current buffer match a buffer model's state."""
    if buf := _get_buffer(handle):
        buf.apply()


def tp_expected_prop_for_type(
        handle: int, lnum: int, type_name: str) -> dict[str, int | str]:
    """Generate the expected property for a given type."""
    if buf := _get_buffer(handle):
        return buf.expected_prop_for_type(
            lnum, type_name.decode('utf-8', errors='ignore'))
    return {}


def tp_expected_prop_list(
        handle: int, lnum: int) -> list[dict[str, int | str]]:
    """Generate the expected property list for a given line."""
    if buf := _get_buffer(handle):
        return buf.expected_prop_list(lnum)
    return {}


def tp_remove_property_from_line(
        handle: int, lnum: int, type_name: str) -> None:
    """Remove a property for a given type from a line."""
    if buf := _get_buffer(handle):
        return buf.remove_property_from_line(
            lnum, type_name.decode('utf-8', errors='ignore'))
    return {}


def tp_dump_state(handle: int) -> None:
    """Dump details of a BufferState."""
    if buf := _get_buffer(handle):
        buf.dump()


def tp_check_buffer_content(handle: int, msg: string = '') -> None:
    """Check that the current buffer matches the modelled state."""
    if buf := _get_buffer(handle):
        buf.check_buffer_contents(add_msg=msg.decode('utf-8', errors='ignore'))


def tp_delete_text(handle: int, lnum: int, col: int, count: int) -> None:
    """Delete characters (including newlines) starting at a given position."""
    if buf := _get_buffer(handle):
        buf.delete_text(lnum, col, count)


def tp_insert_text(handle: int, lnum: int, col: int, text: str) -> None:
    """Insert text starting after a given position."""
    if buf := _get_buffer(handle):
        buf.insert_text(lnum, col, text)


def tp_delete_property_types(handle: int) -> None:
    """Delete all property types set by this model."""
    if buf := _get_buffer(handle):
        buf.delete_property_types()


def tp_format_errors(errors: list[str]) -> None:
    """Print the assertion errors list nicely formatted."""
    r_script = re.compile(r'script ([^]]+)\[([0-9]+)\]')
    r_function = re.compile(r'function ([^ ]+) line ([0-9]+): (.*)')
    r_expected_but_got = re.compile(r'(.*: )?Expected (.+) but got (.+)')

    def split_error_into_lines(error: str) -> Iterator[str]:
        lines = error.split('..')
        if not lines:
            return
        accum = lines.pop(0)
        while lines:
            pending = lines.pop(0)
            pending_is_new_line = pending.startswith('function ')
            if not pending_is_new_line:
                pending_is_new_line = pending.startswith('script ')
            if pending_is_new_line:
                yield accum
                accum = pending
            else:
                accum += '..' + pending
        if accum:
            yield accum

    for err_idx, error in enumerate(errors):
        error = error.decode('utf-8', errors='ignore')
        print(f'Error {err_idx}:')
        lines = list(split_error_into_lines(error))
        to_skip = None
        for i, el in enumerate(lines):
            if to_skip == i:
                to_skip = None
                continue

            m = r_script.match(el)
            if m:
                try:
                    path = Path(m.group(1)).relative_to(Path.cwd())
                except ValueError:
                    path = Path(m.group(1))
                print(f'  {i:2}: {path}:{m.group(2)}')
                continue

            m = r_function.match(el)
            if m:
                print(f'  {i:2}: {m.group(1)}:{m.group(2)}')
                me = r_expected_but_got.match(m.group(3))
                if me:
                    if me.group(1):
                        print(f'        {me.group(1).strip()}')
                    print('        Expected:')
                    print(f'          {me.group(2)}')
                    print('        Actual:')
                    print(f'          {me.group(3)}')
                else:
                    print(f'        {m.group(3)}')
                continue

            print(f'   {i:2}: {el}')


def _get_buffer(handle: int) -> BufferState | None:
    buf = BufferState.instances.get(handle)
    if buf is None:
        assert_report(f'No BufferState instance for handle {handle}')
    return buf
