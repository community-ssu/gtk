from gazpacho.unittest import common

from gazpacho.project import Project

class ApplicationTest(common.GazpachoTest):
    def setUp(self):
        common.GazpachoTest.setUp(self)

    def tearDown(self):        
        while self.app._projects:
            self.app.close_current_project()

        action_group = self.get_project_action_group()
        action_list = action_group.list_actions()
        for action in action_list:
            action_group.remove_action(action)

    def testAddProjectSelf(self):
        self.project.path = "/dummy"
        self.app._add_project(self.project)

        action_group = self.get_project_action_group()
        
        self.assertEqual(len(self.app._projects), 1)
        self.failUnless(self.project in self.app._projects)
        self.assertEqual(len(action_group.list_actions()), 1)
        
    def testAddProject(self):

        action_group = self.get_project_action_group()

        # Add a project
        prj = Project(True, self)
        prj.name = "Dummy.glade"
        prj.path = "/Dummy/1"
        self.app._add_project(prj)
        
        self.assertEqual(len(self.app._projects), 2)
        self.failUnless(prj in self.app._projects)

        #project_action = self.app._ui_manager.get_action('/MainMenu/ProjectMenu/%s' % prj.name)
        #self.assertNotEqual(project_action, None)
        self.assertEqual(len(action_group.list_actions()), 2)
        
        # Add another project with the same name and different path
        prj = Project(True, self)
        prj.name = "Dummy.glade"
        prj.path = "/Dummy/2"
        self.app._add_project(prj)
        
        self.assertEqual(len(self.app._projects), 3)
        self.failUnless(prj in self.app._projects)

        #project_action = self.app._ui_manager.get_action('/MainMenu/ProjectMenu/%s' % prj.name)
        #self.assertNotEqual(project_action, None)
        self.assertEqual(len(action_group.list_actions()), 3)


    def get_project_action_group(self):
        """Convinient function for getting the ProjectAction action
        group."""
        for action_group in self.app._ui_manager.get_action_groups():
            if action_group.get_name() == 'ProjectActions':
                return action_group




    def testValidateWidgetNames(self):
        # Create a dialog which has internal children to see if the
        # validate widget names function can deal with them
        dialog = self.create_gwidget('GtkDialog')
        self.project.add_widget(dialog.gtk_widget)
        
        success = self.app.validate_widget_names(self.project, False)

        self.assertEqual(success, True)

