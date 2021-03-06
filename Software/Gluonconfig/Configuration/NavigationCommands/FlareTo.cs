﻿using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Drawing;
using System.Data;
using System.Linq;
using System.Text;
using System.Windows.Forms;
using Communication.Frames.Incoming;

namespace Configuration.NavigationCommands
{
    public partial class FlareTo : UserControl, INavigationCommandViewer
    {
        private NavigationInstruction ni;

        public FlareTo(NavigationInstruction ni)
        {
            InitializeComponent();
            SetNavigationInstruction(ni);
        }

        private void _cbRelToHome_CheckedChanged(object sender, EventArgs e)
        {
            if (_cbRelToHome.Checked)
                ni.opcode = NavigationInstruction.navigation_command.FLARE_TO_REL;
            else
                ni.opcode = NavigationInstruction.navigation_command.FLARE_TO_ABS;
            SetNavigationInstruction(ni);
        }

#region INavigationCommandViewer Members

        public NavigationInstruction GetNavigationInstruction()
        {

            return ((INavigationCommandViewer)tableLayoutPanel.Controls[0]).GetNavigationInstruction();
        }

        public void SetNavigationInstruction(NavigationInstruction ni)
        {
            this.ni = ni;
            tableLayoutPanel.Controls.Clear();
            if (ni.HasRelativeCoordinates())
                ni.opcode = NavigationInstruction.navigation_command.FLARE_TO_REL;
            else
                ni.opcode = NavigationInstruction.navigation_command.FLARE_TO_ABS;


            if (ni.opcode == NavigationInstruction.navigation_command.FLARE_TO_REL)
            {
                tableLayoutPanel.Controls.Add(new FlareToRel(ni));
                _cbRelToHome.Checked = true;
            }
            else
            {
                tableLayoutPanel.Controls.Add(new FlareToAbs(ni));
                _cbRelToHome.Checked = false;
            }
            this.Width = tableLayoutPanel.Controls[0].Width;
        }

#endregion
    }
}
