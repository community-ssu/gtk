from gazpacho.unittest import common

from gazpacho.command import CommandManager, Command
from gazpacho.command import CommandAddRemoveSignal, CommandChangeSignal

class DummyCommand(Command):
    """A dummy command used for testing."""
    
    def __init__(self, app=None):
        Command.__init__(self, "Dummy", app)

        self.undone = False
        self.redone = False

    def undo(self):
        self.undone = True

    def redo(self):
        self.redone = True

class CommandTestHelper(common.GazpachoTest):
    def setUp(self):
        common.GazpachoTest.setUp(self)
        self.manager = CommandManager(self.app)
        
class CommandManagerTest(CommandTestHelper):

    def testUndoEmpty(self):
        # Test what happens when calling CommandManager.undo() when
        # the undo list is empty.
        try:
            self.manager.undo(self.project)
        except:
            self.fail()

    def testUndo(self):
        # Test that CommandManager.undo() works as it should.
        cmd = DummyCommand()
        self.project.undo_stack.push_undo(cmd)
        self.manager.undo(self.project)        
        self.assertEqual(cmd.undone, True)
        assert not self.project.undo_stack.has_undo()

    def testRedoEmpty(self):
        # Test what happens when calling CommandManager.redo() when
        # the redo list is empty"""
        try:
            self.manager.redo(self.project)
        except:
            self.fail()

    def testRedo(self):
        # Test that CommandManager.redo() works as it should.
        cmd = DummyCommand()
        self.project.undo_stack.push_undo(cmd)
        self.project.undo_stack.pop_undo()
        self.manager.redo(self.project)
        self.assertEqual(cmd.redone, True)
        assert not self.project.undo_stack.has_redo()


    def testCopy(self):
        # Test CommandManager.copy()
        gwidget = self.create_gwidget("GtkLabel")
        self.manager.copy(gwidget)

        clipboard = self.app.get_clipboard()
        clipboard.selected = None
        item = clipboard.get_selected_item()
        self.assertEqual(item.name, gwidget.name)

class SignalCommandTest(common.GazpachoTest):
    
    def setUp(self):
        common.GazpachoTest.setUp(self)
        self.manager = CommandManager(self.app)
        
        self.window = self.create_gwidget('GtkWindow')
        self.signal = {'name': 'active-default',
                       'handler': 'my_handler',
                       'after': False}

    def _assert_has_signal(self, signal = None):
        if not signal:
            signal = self.signal
        
        test_signals = self.window.signals.get(signal['name'])
        assert len(test_signals) == 1
        assert test_signals[0]['handler'] == signal['handler']
        assert test_signals[0]['after'] == signal['after']

    def _assert_no_signal(self, signal = None):
        if not signal:
            signal = self.signal
            
        test_signals = self.window.signals.get(self.signal['name'])
        assert not test_signals

    def testAddSignalCommand(self):
        # Test the add signal command
        cmd = CommandAddRemoveSignal(True,
                                     self.signal,
                                     self.window,
                                     "Info",
                                     self.app)
        cmd.execute()
        self._assert_has_signal()
        
        cmd.undo()
        self._assert_no_signal()
        
        cmd.redo()
        self._assert_has_signal()

    def testAddSignalManager(self):
        # Test add signal in the command manager
        self.manager.add_signal(self.window, self.signal)
        self._assert_has_signal()

    def testRemoveSignal(self):
        # Test the remove signal command
        self.window.do_add_signal_handler(self.signal)

        cmd = CommandAddRemoveSignal(False,
                                     self.signal,
                                     self.window,
                                     "Info",
                                     self.app)        

        cmd.execute()
        self._assert_no_signal()
        
        cmd.undo()
        self._assert_has_signal()

        cmd.redo()
        self._assert_no_signal()

    def testRemoveSignalManager(self):
        # Test remove signal in the command manager
        self.window.do_add_signal_handler(self.signal)
        self.manager.remove_signal(self.window, self.signal)
        self._assert_no_signal()
        
    def testChangeSignal(self):
        # Test the change signal command
        self.window.do_add_signal_handler(self.signal)

        new_signal = {'name': self.signal['name'],
                      'handler': 'new_handler',
                      'after': True}
                      
        
        cmd =  CommandChangeSignal(self.window,
                                   self.signal,
                                   new_signal,
                                   "Info",
                                   self.app)
        
        cmd.execute()
        self._assert_has_signal(new_signal)

        cmd.undo()
        self._assert_has_signal()

        cmd.redo()
        self._assert_has_signal(new_signal)
        
    def testChangeSignalManager(self):
        # Test change signal in the command manager
        self.window.do_add_signal_handler(self.signal)
        new_signal = {'name': self.signal['name'],
                      'handler': 'new_handler',
                      'after': True}

        self.manager.change_signal(self.window, self.signal, new_signal)
        self._assert_has_signal(new_signal)
            
