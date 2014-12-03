#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2014 TAS
# Copyright © 2014 CNES
#
#
# This file is part of the OpenSAND testbed.
#
#
# OpenSAND is free software : you can redistribute it and/or modify it under the
# terms of the GNU General Public License as published by the Free Software
# Foundation, either version 3 of the License, or (at your option) any later
# version.
#
# This program is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY, without even the implied warranty of MERCHANTABILITY or FITNESS
# FOR A PARTICULAR PURPOSE. See the GNU General Public License for more
# details.
#
# You should have received a copy of the GNU General Public License along with
# this program. If not, see http://www.gnu.org/licenses/.
#
#

# Author: Julien BERNARD / <jbernard@toulouse.viveris.com>

"""
conf_event.py - the events on configuration tab
"""

import gtk
import gobject

from opensand_manager_gui.view.conf_view import ConfView
from opensand_manager_gui.view.popup.infos import error_popup, yes_no_popup
from opensand_manager_core.my_exceptions import XmlException, ConfException
from opensand_manager_gui.view.popup.advanced_dialog import AdvancedDialog

class ConfEvent(ConfView) :
    """ Events on configuration tab """

    def __init__(self, parent, model, manager_log):
        ConfView.__init__(self, parent, model, manager_log)

        self._modif = False
        self._previous_img = ''
        # update the image
        self.refresh_description()
        # hide DVB part at the moment
        widget = self._ui.get_widget('frame_dvb')
        widget.hide_all()

        gobject.idle_add(self.enable_conf_buttons, False)

    def close(self):
        """ close the configuration tab """
        if self._timeout_id != None :
            gobject.source_remove(self._timeout_id)
            self._timeout_id = None
        self._log.debug("Conf Event: close")
        self._log.debug("Conf Event: description refresh joined")
        self._log.debug("Conf Event: closed")

    def activate(self, val):
        """ 'activate' signal handler """
        if val == False and self._timeout_id != None :
            gobject.source_remove(self._timeout_id)
            self._timeout_id = None
        elif val and self._timeout_id is None:
            # refresh immediatly then periodically
            self.update_lan_adaptation()
            self._timeout_id = gobject.timeout_add(1000,
                                                   self.update_lan_adaptation)

        if val == False:
            self._modif = False
        else:
            self._modif = True
            self._previous_img = ''
            # update the image
            self.refresh_description()

    def refresh_description(self):
        """ refresh the description and image """
        if not self._modif:
            return True

        gobject.idle_add(self.refresh_gui)

    def refresh_gui(self):
        """ the part of the refreshing that modifies GUI
            (should be used with gobject.idle_add outside gtk ) """
        empty = 0

        # payload type
        if self.is_button_active('transparent'):
            empty = 80
            self._ui.get_widget("repr_stack_sat_encap").hide_all()
            self._ui.get_widget("repr_stack_label_gw").set_markup("<b>GW</b>")
        elif self.is_button_active('regenerative'):
            empty = 40
            self._ui.get_widget("repr_stack_sat_encap").show_all()
            self._ui.get_widget("repr_stack_label_gw").set_markup("<b>ST</b>")

        widget = self._ui.get_widget("repr_stack_sat_empty_ip")
        widget.set_size_request(100, empty)

        # emission standard
#        if self.is_button_active('DVB-RCS'):
#        elif self.is_button_active('DVB-S2'):

        # output encapsulation scheme
        widget = self._ui.get_widget("repr_stack_label_st_encap")
        widget.set_has_tooltip(True)
        widget.connect("query-tooltip", self.build_encap_tooltip)
        widget = self._ui.get_widget("repr_stack_label_gw_encap")
        widget.set_has_tooltip(True)
        widget.connect("query-tooltip", self.build_encap_tooltip)
        if self.is_button_active('regenerative'):
            widget = self._ui.get_widget("repr_stack_label_sat_encap")
            widget.set_has_tooltip(True)
            widget.connect("query-tooltip", self.build_encap_tooltip)

        # physical layer
        widget1 = self._ui.get_widget("repr_stack_st_physical_layer")
        widget2 = self._ui.get_widget("repr_stack_sat_physical_layer")
        widget3 = self._ui.get_widget("repr_stack_gw_physical_layer")
        widget = self._ui.get_widget('enable_physical_layer')
        if widget.get_active():
            widget1.set_size_request(100, 40)
            widget2.set_size_request(100, 40)
            widget3.set_size_request(100, 40)
            widget1.show_all()
            widget2.show_all()
            widget3.show_all()
        else:
            widget1.set_size_request(100, 0)
            widget2.set_size_request(100, 0)
            widget3.set_size_request(100, 0)
            widget1.hide_all()
            widget2.hide_all()
            widget3.hide_all()

        self._drawing_area.queue_draw()

    def build_encap_tooltip(self, widget, x, y, keyboard_mode, tooltip):
        """ show the encapsulation stack in a tooltip """
        box = gtk.VBox()
        hbox = gtk.HBox()
        vbox_in = gtk.VBox()
        vbox_out = gtk.VBox()
        in_stack = self._in_stack.get_stack()
        out_stack = self._out_stack.get_stack()
        in_label = None
        out_label = None
        packed = False
        for pos in range(max(len(in_stack), len(out_stack))):
            if pos < len(out_stack):
                out_frame = gtk.AspectFrame()
                out_label = gtk.Label()
                out_label.set_size_request(70, 45)
                out_label.set_text(out_stack[str(pos)])
                out_frame.add(out_label)
            elif out_label is not None:
                heigth = out_label.get_size_request()[1]
                out_label.set_size_request(70, heigth + 50)
            if pos < len(in_stack):
                if pos < len(out_stack):
                    if out_stack[str(pos)] == in_stack[str(pos)]:
                        if packed:
                            hbox.pack_end(vbox_in)
                            sep = gtk.VSeparator()
                            hbox.pack_end(sep)
                            hbox.pack_end(vbox_out)
                            hbox.set_child_packing(vbox_in, expand=False,
                                                   fill=False, padding=0,
                                                   pack_type=gtk.PACK_END)
                            hbox.set_child_packing(sep, expand=False,
                                                   fill=False, padding=0,
                                                   pack_type=gtk.PACK_END)
                            hbox.set_child_packing(vbox_out, expand=False,
                                                   fill=False, padding=0,
                                                   pack_type=gtk.PACK_END)
                            box.pack_start(hbox)

                            hbox = gtk.HBox()
                            vbox_in = gtk.VBox()
                            vbox_out = gtk.VBox()
                            in_label = None
                            packed = False

                        box.pack_start(out_frame)
                        out_label.set_size_request(145, 45)
                        box.set_child_packing(out_frame, expand=False,
                                              fill=False, padding=0,
                                              pack_type=gtk.PACK_START)
                        out_label = None
                        continue
                    else:
                        vbox_out.pack_start(out_frame)
                        vbox_out.set_child_packing(out_frame, expand=False,
                                                   fill=False, padding=0,
                                                   pack_type=gtk.PACK_START)
                        packed = True
                in_frame = gtk.AspectFrame()
                in_label = gtk.Label()
                in_label.set_size_request(70, 45)
                in_label.set_text(in_stack[str(pos)])
                in_frame.add(in_label)
                vbox_in.pack_start(in_frame)
                vbox_in.set_child_packing(in_frame, expand=False,
                                          fill=False, padding=0,
                                          pack_type=gtk.PACK_START)
                packed = True
            elif in_label is not None:
                heigth = in_label.get_size_request()[1]
                in_label.set_size_request(70, heigth + 50)
            elif pos < len(out_stack):
                vbox_out.pack_start(out_frame)
                vbox_out.set_child_packing(out_frame, expand=False,
                                           fill=False, padding=0,
                                           pack_type=gtk.PACK_START)
                packed = True

        hbox.pack_end(vbox_in)
        sep = gtk.VSeparator()
        hbox.pack_end(sep)
        hbox.pack_end(vbox_out)
        hbox.set_child_packing(vbox_in, expand=False,
                               fill=False, padding=0,
                               pack_type=gtk.PACK_END)
        hbox.set_child_packing(sep, expand=False,
                               fill=False, padding=0,
                               pack_type=gtk.PACK_END)
        hbox.set_child_packing(vbox_out, expand=False,
                               fill=False, padding=0,
                               pack_type=gtk.PACK_END)
        box.pack_start(hbox)
        box.show_all()
        tooltip.set_custom(box)

        return True

    def enable_conf_buttons(self, enable=True):
        """ make apply and cancel buttons sensitive or not """
        self._modif = True

        # check if OpenSAND is running
#        if self._model.is_running():
#            enable = False

        self.refresh_description()

        self._ui.get_widget('save_conf').set_sensitive(enable)
        self._ui.get_widget('undo_conf').set_sensitive(enable)

    def on_regenerative_button_clicked(self, source=None, event=None):
        """ actions performed when regenerative is selected """
        self.enable_conf_buttons()
        # update the protocol stacks
        self._out_stack.set_payload_type('regenerative')
        self._in_stack.set_payload_type('regenerative')
        widget = self._ui.get_widget('label_in_encap')
        widget.set_markup('Downlink')
        widget = self._ui.get_widget('label_out_encap')
        widget.set_markup('Uplink')
        widget = self._ui.get_widget('label_emission_standard')
        widget.set_markup('Uplink standard')

        # disable DVB-S2 emission standard on ST because it is not
        # implemented in regenerative mode yet
        widget = self._ui.get_widget('DVB-S2')
        widget.set_sensitive(False)


    def on_transparent_button_clicked(self, source=None, event=None):
        """ actions performed when transparent is selected """
        self.enable_conf_buttons()
        # update the protocol stacks
        self._out_stack.set_payload_type('transparent')
        self._in_stack.set_payload_type('transparent')
        widget = self._ui.get_widget('label_in_encap')
        widget.set_markup('Forward link')
        widget = self._ui.get_widget('label_out_encap')
        widget.set_markup('Return link')
        widget = self._ui.get_widget('label_emission_standard')
        widget.set_markup('Return link standard')

        # disable DVB-S2 emission standard on ST because it is not
        # implemented in transparent mode yet
        widget = self._ui.get_widget('DVB-S2')
        widget.set_sensitive(False)


    def on_dvb_rcs_button_clicked(self, source=None, event=None):
        """ actions performed when DVB-RCS is selected """
        self.enable_conf_buttons()
#        self.populate_dama(self._dama_rcs)

    def on_dvb_s2_button_clicked(self, source=None, event=None):
        """ actions performed when DVB-S2 is selected """
        self.enable_conf_buttons()
#        self.populate_dama(self._dama_s2)

#    def populate_dama(self, dama_list):
#        """ populate the DAMA combobox """
#        combo = self._ui.get_widget('dama_box')
#        combo.clear()
#        store = gtk.ListStore(gobject.TYPE_STRING)
#        for name in dama_list:
#            store.append([name])
#        combo.set_model(store)
#        cell = gtk.CellRendererText()
#        combo.pack_start(cell, True)
#        combo.add_attribute(cell, 'text', 0)
#        combo.set_active(0)
        
    def on_button_clicked(self, source=None, event=None):
        """ 'clicked' event on teminal type buttons """
        self.enable_conf_buttons()

    def on_enable_physical_layer_toggled(self, source=None, event=None):
        """ 'toggled' event on enable button """
        self.enable_conf_buttons()

    def on_undo_conf_clicked(self, source=None, event=None):
        """ reload conf from the ini file """
        try:
            self.update_view()
        except ConfException as msg:
            error_popup(str(msg))
        self.enable_conf_buttons(False)

    def on_save_conf_clicked(self, source=None, event=None):
        """ save the new configuration in the ini file """
        # retrieve global parameters

        # payload type
        if(self.is_button_active('transparent') == True):
            payload_type = 'transparent'
        elif(self.is_button_active('regenerative') == True):
            payload_type = 'regenerative'
        else:
            return
        config = self._model.get_conf()
        config.set_payload_type(payload_type)

        # emission standard
        if(self.is_button_active('DVB-RCS') == True):
            emission_std = 'DVB-RCS'
        elif(self.is_button_active('DVB-S2') == True):
            emission_std = 'DVB-S2'
        else:
            return
        config.set_emission_std(emission_std)

        # dama
#        widget = self._ui.get_widget('dama_box')
#        model = widget.get_model()
#        active = widget.get_active_iter()
#        config.set_dama(model.get_value(active, 0))

        # check stacks with modules conditions
        modules = self._model.get_encap_modules()
        stack = self._out_stack.get_stack()
        if len(stack) == 0:
            error_popup("Out stack is empty !")
            return
        pos = max(stack.keys())
        if not modules[stack[pos]].get_condition(emission_std.lower()):
            error_popup("Module %s does not support %s link" %
                        (stack[pos], emission_std))
            return
        if modules[stack[pos]].get_condition('mandatory_down'):
            error_popup("Module %s need a lower encapsulation module" %
                        stack[pos])
            return
        stack = self._in_stack.get_stack()
        if len(stack) == 0:
            error_popup("In stack is empty !")
            return
        pos = max(stack.keys())
        if not modules[stack[pos]].get_condition('dvb-s2'):
            error_popup("Module %s does not support DVB-S2 link" % stack[pos])
            return
        if modules[stack[pos]].get_condition('mandatory_down'):
            error_popup("Module %s need a lower encapsulation module" %
                        stack[pos])
            return
        # for regenerative, check that the output stack is the same as the upper
        # input stack because satellite does not decapssulate
        if self.is_button_active('regenerative'):
            if len(set(self._out_stack.get_stack().items()) - \
                   set(self._in_stack.get_stack().items())) != 0:
                error_popup("In regenerative mode, the downlink stack should "
                            "be at least the same as the uplink one")
                # TODO we could update the down stack with the up stack in
                # protocol_stack.py to avoid this error
                return

        # lan adaptation schemes
        # check that the last module in the stack is the same for all hosts
        if self._model.get_adv_mode():
            previous = None
            last = None
            gw_stack = None
            other_stacks = []
            for host in self._lan_stacks:
                stack = self._lan_stacks[host].get_stack()
                if host.get_name() == 'gw':
                    gw_stack = stack
                else:
                    other_stacks.append(stack)
                # get the last module of the stack that is not a header modification
                # module
                for pos in range(len(stack)):
                    mod = stack[str(len(stack) - 1 - pos)]
                    # header modification plugins are generally in global plugins
                    host_modules = host.get_lan_adapt_modules()
                    if mod in host_modules and  \
                       not host_modules[mod].get_condition("header_modif"):
                        last = mod
                        break
                if previous is not None and last != previous:
                    error_popup("The last module of the LAN stack should be the "
                                "same for each host (except for header modifications)")
                    return
                previous = last
            # check that the GW stack is at least the same as other
            for stack in other_stacks:
                if len(set(gw_stack.values()) - set(stack.values())) != 0:
                    error_popup("The hosts stack should be at least the same as "
                                "for GW")
                    return
        for host in self._lan_stacks:
            if self._model.get_adv_mode():
                host.set_lan_adaptation(self._lan_stacks[host].get_stack())
            else:
                host.set_lan_adaptation(self._lan_stack_base.get_stack())

        # output encapsulation scheme
        config.set_return_up_encap(self._out_stack.get_stack())

        # input encapsulation scheme
        config.set_forward_down_encap(self._in_stack.get_stack())

        # enable physical layer
        widget = self._ui.get_widget('enable_physical_layer')
        if widget.get_active():
            config.set_enable_physical_layer("true")
        else:
            config.set_enable_physical_layer("false")

        try:
            config.save()
        except XmlException, error:
            error_popup(str(error), error.description)
            self.on_undo_conf_clicked()
        except ConfException, error:
            error_popup(str(error))
            self.on_undo_conf_clicked()

        self.update_view()
        self.enable_conf_buttons(False)

#    def on_dama_box_changed(self, source=None, event=None):
#        """ 'change' event callback on dama box """
#        self.enable_conf_buttons()

    def on_advanced_conf_clicked(self, source=None, event=None):
        """ display the advanced window """
        if self.is_modified():
            text =  "Save current configuration ?"
            ret = yes_no_popup(text,
                               "Save Configuration - OpenSAND Manager",
                                gtk.STOCK_DIALOG_INFO)
            if ret == gtk.RESPONSE_YES:
                self.on_save_conf_clicked()
            else:
                try:
                    self.update_view()
                except ConfException as msg:
                    error_popup(str(msg))
        window = AdvancedDialog(self._model, self._log, self.update_view)
        window.go()
        try:
            self.enable_conf_buttons(False)
        except ConfException as msg:
            error_popup(str(msg))


