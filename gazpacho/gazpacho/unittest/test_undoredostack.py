from gazpacho.project import UndoRedoStack
from gazpacho.command import Command
from gazpacho.unittest import common

class DummyCommand(Command):
    """A dummy command used for testing."""
    
    def __init__(self, description = None):
        Command.__init__(self, description, None)

        self.unifiable = False
        self.collapsed = False

    def unifies(self, other):
        return self.unifiable
    
    def collapse(self, other):
        if self.unifies:
            self.collapsed = True

class UndoRedoStackTest(common.GazpachoTest):

    def setUp(self):
        common.GazpachoTest.setUp(self)
        self.stack = UndoRedoStack()

    def testPushUndo(self):
        command = DummyCommand()
        self.stack.push_undo(command)
        assert self.stack.has_undo(), "Should have a command on the undo stack."

        command = DummyCommand()
        self.stack.push_undo(command)
        undos = self.stack.get_undo_commands()
        self.assertEqual(len(undos), 2)

    def testPushUndoUnify(self):
        
        command0 = DummyCommand()
        command0.unifiable = True
        self.stack.push_undo(command0)

        command1 = DummyCommand()
        command1.unifiable = True
        self.stack.push_undo(command1)

        undos = self.stack.get_undo_commands()
        self.assertEqual(len(undos), 1)
        assert command0.collapsed, "Command should have been collapsed"

    def testPushUndoWithRedo(self):
        for i in range(3):
            command = DummyCommand(i)
            self.stack.push_undo(command)

        self.stack.pop_undo()
        assert self.stack.has_redo()

        # Make sure the new command replaces the redo commands
        command = DummyCommand(3)
        self.stack.push_undo(command)
        assert not self.stack.has_redo()
        
        undos = self.stack.get_undo_commands()
        self.assertEqual(len(undos), 3)

        # Make sure it ends up in the right place
        undo = self.stack.pop_undo()
        self.assertEqual(command.description, undo.description)

    def testPopUndo(self):
        for i in range(2):
            command = DummyCommand(i)
            self.stack.push_undo(command)

        # Pop the first undo
        undo = self.stack.pop_undo()
        assert undo.description == 1, "Should have gotten the last command"
        assert self.stack.has_undo(), "Should have undo"
        assert self.stack.has_redo(), "Should have redo"

        # Pop the last undo
        undo = self.stack.pop_undo()
        assert undo.description == 0, "Should have gotten the first command"
        assert not self.stack.has_undo(), "Should not have undo"
        assert self.stack.has_redo(), "Should have redo"

        # Pop when empty
        undo = self.stack.pop_undo()
        assert not undo, "Should not have gotten a redo since there are none."

    def testGetUndoInfo(self):
        command0 = DummyCommand(0, "Zero")
        command1 = DummyCommand(1, "One")
        self.stack.push_undo(command0)
        self.stack.push_undo(command1)

        info = self.stack.get_undo_info()
        self.assertEqual(info, command1.description)

        self.stack.pop_undo()
        info = self.stack.get_undo_info()
        self.assertEqual(info, command0.description)

    def testGetRedo(self):
        for i in range(2):
            command = DummyCommand(i)
            self.stack.push_undo(command)

        for i in range(2):
            self.stack.pop_undo()

        assert self.stack.has_redo()

        # Pop the first redo
        redo = self.stack.pop_redo()
        assert redo.description == 0, "Should have gotten the first command"
        assert self.stack.has_undo(), "Should have undo"
        assert self.stack.has_redo(), "Should have redo"

        # Pop the last redo
        redo = self.stack.pop_redo()
        assert redo.description == 1, "Should have gotten the first command"
        assert not self.stack.has_redo(), "Should not have undo"
        assert self.stack.has_undo(), "Should have redo"

        # Pop when empty
        redo = self.stack.pop_redo()
        assert not redo, "Should not have gotten a redo since there are none."

    def testGetUndoInfo(self):
        command0 = DummyCommand("Zero")
        command1 = DummyCommand("One")
        self.stack.push_undo(command0)
        self.stack.push_undo(command1)
        self.stack.pop_undo()
        self.stack.pop_undo()

        info = self.stack.get_redo_info()
        self.assertEqual(info, command0.description)

        self.stack.pop_redo()
        info = self.stack.get_redo_info()
        self.assertEqual(info, command1.description)

