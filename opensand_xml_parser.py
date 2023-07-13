#!/usr/bin/env python2
# -*- coding: utf-8 -*-

#
#
# OpenSAND is an emulation testbed aiming to represent in a cost effective way a
# satellite telecommunication system for research and engineering activities.
#
#
# Copyright © 2019 TAS
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
# Author: Joaquin MUGUERZA / <jmuguerza@toulouse.viveris.com>

"""
opensand_xml_parser.py - the XML parser and builder for OpenSAND configuration

  The OpenSAND configuration XML format is:
  <?xml version="1.0" encoding="UTF-8"?>
  <configuration component='COMPO'>
    <SECTION>
      <KEY>VAL</KEY>
      <TABLES>
        <TABLE ATTRIBUTE1="VAL1" ATTRIBUTE2="VAL2" />
        <TABLE ATTRIBUTE1="VAL1'" ATTRIBUTE2="VAL2'" />
      </TABLES>
    </SECTION>
  </configuration>
"""

import re
from copy import deepcopy

from lxml import etree
from my_exceptions import XmlException
from utils import (SPOT, ID, GW) 


NAMESPACES = {"xsd":"http://www.w3.org/2001/XMLSchema"}


class XmlParser:
    """ XML parser for OpenSAND configuration """
    def __init__ (self, xml, xsd):
        self._tree = None
        self._filename = xml
        self._xsd = xsd
        self._xsd_parser = None
        self._common_xsd = None
        self._schema = None
        try:
            self._tree = etree.parse(xml)
            self._schema = etree.XMLSchema(etree.parse(xsd))
            self._xsd_parser = etree.parse(xsd)
            common_path = self._xsd_parser.xpath("//xsd:include/@schemaLocation",
                                                 namespaces=NAMESPACES)
            if len(common_path) == 1:
                self._common_xsd = etree.parse(common_path[0])
        except IOError as err:
            raise
        except (etree.XMLSyntaxError, etree.XMLSchemaParseError) as err:
            raise XmlException(err.error_log.last_error.message)

        if not self._schema.validate(self._tree):
            raise XmlException(self._schema.error_log.last_error.message)

        root = self._tree.getroot()
        

    def get_sections(self):
        """ get all the sections in the configuration file """
        root = self._tree.getroot()
        return [sect for sect in root.iterchildren()
                     if sect.tag is not etree.Comment]

    def get_name(self, node):
        """ get the name of any XML node """
        return node.tag

    def get_keys(self, section):
        """ get all the keys in a section """
        return [key for key in section.iterchildren()
                    if key.tag is not etree.Comment]

    def is_table(self, key):
        """ check if the key is a table one """
        return len(key) > 0

    def get_value(self, key):
        """ get the value stored in a key """
        if self.is_table(key):
            raise XmlException("Key is a table, cannot get its value")
        return key.text

    def get_table_elements(self, key):
        """ get the list of element in a table """
        return [elt for elt in key.iterchildren()
                    if elt.tag is not etree.Comment]

    def get_element_content(self, element):
        """ get a dictionnary containing the attributes and values of a list
            element """
        return element.attrib

    def get_path(self, elt):
        """ get the xpath of an XML attribute or key """
        return self._tree.getpath(elt)

    def set_value(self, val, path, att=None):
        """ set the value of a key """
        elt = self._tree.xpath(path)
        if len(elt) != 1:
            if att is None:
                raise XmlException("[set_value() ]wrong path %s" % path)
            # this happens for example when we add a line and modify the first
            # line in a table, the first line won't have an index and so we ask
            # for a path that may return more than one elements.
            # Thus, choose the first one
            elt = [elt[0]]


        if att is not None:
            if not self.is_table(elt):
                raise XmlException("wrong path: %s is not a table" % elt.tag)
            att_list = self.get_element_content(elt[0])
            if not att in att_list:
                raise XmlException("wrong path: %s is not a valid attribute" %
                                   att)
            att_list[att] = str(val)
        else:
            elt[0].text = str(val)

    def set_values(self, val, xpath, attribute):
        """ set a value on one or more paths """
        elts = self.get_all(xpath)
        if len(elts) < 1:
            raise XmlException("Cannot find %s path" % xpath)

        for elem in elts:
            att_list = self.get_element_content(elem)
            if not attribute in att_list:
                raise XmlException("wrong path: %s is not a valid attribute" %
                                   attribute)
            att_list[attribute] = val

    def get(self, xpath):
        """ get a XML element with its path """
        if not self._tree:
            self._tree = etree.parse(self._filename)
        elts = self._tree.xpath(xpath)
        if len(elts) == 1:
            return elts[0]
        return None

    def get_all(self, xpath):
        """ get all matching elements """
        return self._tree.xpath(xpath)

    def add_element(self, xpath, elt, pos=0):
        """ add an element to path (as a copy) """      
        if len(xpath.split('@')) > 1:
            return
    

        parent = self.get('/'.join(xpath.split('/')[:-1]))
        if parent is not None:
            new = deepcopy(elt)
            parent.insert(pos, new)

    def del_element(self, xpath):
        """ delete an element from its path """
        elts = self._tree.xpath(xpath)
        if len(elts) == 1:
            elt = elts[0]
            elt.getparent().remove(elt)

    def add_spot(self, spot_id):
        """ add a spot in the table identified its path """
        for section in self.get_sections():
            gws = []
            spots = [spot_id]
            for child in section.iterchildren():
                if child.tag == SPOT:
                    if child.get(GW) is not None:
                        if child.get(GW) in gws and \
                           child.get(ID) in spots:
                            break
                        elif child.get(GW) in gws and \
                             child.get(ID) not in spots:
                            spots.append(child.get(ID))
                            continue
                            
                        gws.append(child.get(GW))
                        spots.append(child.get(ID))
                       
                    #TODO: new spot shouldn't be a copy of previous spot,
                    # it should contain the new configuration
                    new = deepcopy(child)
                    new.set(ID, spot_id)
                    child.addnext(new)
                    
                    if child.get(GW) is None:
                        break

    def remove_spot(self, spot_id):
        """ remove spots from the configuration file """
        for section in self.get_sections():
             for child in section.getchildren():
                 if child.tag == SPOT and child.get(ID) == spot_id:
                     section.remove(child)

    def add_gw(self, xpath, gw_id):
        """ add a spot in the table identified its path """
        sections = self._tree.xpath(xpath)
        if len(sections) != 1:
            raise XmlException("wrong path: %s is not valid" % xpath)
        section = sections[0]
        gws = [gw_id]
        spots = []
        for child in section.getchildren():
            if child.tag == SPOT:
                if child.get(GW) is not None and\
                   child.get(ID) is not None :
                    if (child.get(GW) in gws and \
                       child.get(ID) in spots) or \
                       child.get(GW) == gw_id:
                        break
                    elif child.get(GW) not in gws and \
                         child.get(ID) in spots:
                        gws.append(child.get(GW))
                        continue
                        
                    spots.append(child.get(ID))
                    
                    new = deepcopy(child)
                    new.set(GW, gw_id)
                    child.addnext(new)
             
            elif child.tag == GW:
                if child.get(ID) is not None :
                    if child.get(ID) == gw_id:
                        break
                        
                    new = deepcopy(child)
                    new.set(ID, gw_id)
                    child.addnext(new)

    def remove_gw(self, xpath, gw_id):
        """ add a spot in the table identified its path """
        sections = self._tree.xpath(xpath)
        if len(sections) != 1:
            raise XmlException("wrong path: %s is not valid" % xpath)
        section = sections[0]
        for child in section.getchildren():
            if (child.tag == SPOT and child.get(GW) == gw_id) or \
               (child.tag == GW and child.get(ID) == gw_id):
                section.remove(child)

    def add_host(self, tal_id):
        """ add a new host for necessary configurations """
        return

    def remove_host(self, tal_id):
        """ remove a host from necesarry configuration """
        return

    def add_line(self, xpath):
        """ add a line in the table identified its path """
        tables = self._tree.xpath(xpath)
        if len(tables) != 1:
            raise XmlException("wrong path: %s is not valid" % xpath)
        table = tables[0]
        children = table.getchildren()
        if len(children) == 0:
            raise XmlException("wrong path: %s is not a table" % xpath)
        i = 0
        # copy the first line for the base, but skip comments
        while(i < len(children)):
            child = children[i]
            i += 1
            if not child.tag is etree.Comment:
                break
        new = deepcopy(child)
        table.append(new)

    def create_line(self, attributes, key, xpath):
        """ create a new line in a table """
        table = self.get(xpath)
        if table is None:
            raise XmlException("wrong path: %s is not valid" % xpath)
        table.append(etree.Element(key, attributes))

    def remove_line(self, xpath, line = 0):
        """ remove a line in the table identified by its path """
        tables = self._tree.xpath(xpath)
        if len(tables) != 1:
            raise XmlException("wrong path: %s is not valid" % xpath)
        table = tables[0]
        children = table.getchildren()
        if len(children) == 0:
            raise XmlException("wrong path: %s is not a table" % xpath)
        i = 0
        while(i < len(children)):
            child = children[i]
            if not child.tag is etree.Comment:
                child = children[i+line]
                break
            i += 1
        table.remove(child)

    def write(self):
        """ write the new configuration in file """
        if not self._schema.validate(self._tree):
            error = self._schema.error_log.last_error.message
            self.__init__(self._filename, self._xsd)
            raise XmlException("the new values are incorrect: %s" %
                               error)

        with open(self._filename, 'w') as conf:
            str = etree.tostring(self._tree, pretty_print=True,
                                      encoding=self._tree.docinfo.encoding,
                                      xml_declaration=True)
            
            conf.write(str.decode(self._tree.docinfo.encoding))

        # reset the xml else when removing spots, sometimes it leads to segfault
        self.__init__(self._filename, self._xsd)

    def get_file_paths(self):
        """ get the default and source values for all files to be able
            to get them in current scenario """
        files = []
        # get elements of type file
        nodes = self.get_file_elements("element")
        for node in nodes:
            elems = self.get_all("//%s" % node)
            for elem in elems:
                default = self.get_doc_param('/default', self.get_name(elem))
                source = self.get_doc_param('/source', self.get_name(elem))
                source = self.adapt_filename(source, elem)
                files.append((default, source))
        # get attributes of type file
        nodes = self.get_file_elements("attribute")
        for node in nodes:
            elems = self.get_all("//*[@%s]" % node)
            for elem in elems:
                default = self.get_doc_param('/default', node,
                                             self.get_name(elem))
                source = self.get_doc_param('/source', node,
                                            self.get_name(elem))
                path = self.get_path(elem)
                # get line ID in path
                pos = path.rfind('[')
                line = path[pos:].strip('[]')
                source = self.adapt_filename(source, elem, line)
                files.append((default, source))
        return files

    def get_file_sources(self):
        """ get the file elements depending on hosts and xpath and their
            associated source value """
        files = {}
        # get elements of type file
        nodes = self.get_file_elements("element")
        for node in nodes:
            elems = self.get_all("//%s" % node)
            for elem in elems:
                source = self.get_doc_param('/source', self.get_name(elem))
                source = self.adapt_filename(source, elem)
                files[self.get_path(elem)] = source
        # get attributes of type file
        nodes = self.get_file_elements("attribute")
        for node in nodes:
            elems = self.get_all("//*[@%s]" % node)
            for elem in elems:
                source = self.get_doc_param('/source', node,
                                            self.get_name(elem))
                path = self.get_path(elem)
                # get line ID in path
                pos = path.rfind('[')
                line = path[pos:].strip('[]')
                source = self.adapt_filename(source, elem, line)
                files["%s/@%s" % (path, node)] = source
        return files

    def adapt_filename(self, source, elem, line=""):
        """ add spot, gw and line extension """
        # try to rename file.ext into file_spotX_gwY_line.ext to keep ext

        # remove spotX or spot_X, in name
        name = re.sub("[_]?spot[_]?[0-9]*", "", source)
        # remove gwX, gw_X in name
        name = re.sub("[_]?gw[_]?[0-9]*", "", name)
        # replace _X. by . in name (this correspond to line in tables)
        name = re.sub("[_]([1-9]?)*\.", ".", name)
        # remove _X in end of name (this correspond to line in tables)
        name = re.sub("[_]([1-9]?)*$", "", name)
        ext = ""
        try:
            (name, ext) = name.rsplit(".", 1)
        except ValueError:
            pass
        if line == "":
            spot_elem = elem.getparent()
        else:
            line = "_" + line
            spot_elem = elem.getparent().getparent()
        if spot_elem.tag == SPOT:
            spot = "_spot%s" % spot_elem.get(ID)
            name += spot
            if spot_elem.get(GW) is not None:
                gw = "_gw%s" % spot_elem.get(GW)
                name += gw
        if ext != "":
            source = "%s%s.%s" % (name, line, ext)
        else:
            source = "%s%s" % (name, line)
        return source


    #### functions for XSD parsing ###

    def get_type(self, name):
        """ get an element type in the XSD document """
        elem = self.get_element(name, True)
        if elem is None:
            return None

        elem_type = elem.get("type")
        if elem_type is None:
            return None

        if elem_type.startswith("xsd:"):
            ret = self.parse_numeric(elem_type)
            if ret is not None:
                return ret
            else:
                return {"type": elem_type.replace("xsd:", "", 4)}
        elif elem_type == "file":
            return {"type": "file"}
        else:
            return self.get_simple_type(elem_type)

    def get_attribute_type(self, name, parent_name):
        """ get an attribute type in the XSD document """
        attribute = self.get_attribute(name, parent_name)
        if attribute is None:
            return None

        att_type = attribute.get("type")
        if att_type is None:
            return None

        if att_type.startswith("xsd:"):
            ret = self.parse_numeric(att_type)
            if ret is not None:
                return ret
            else:
                return {"type": att_type.replace("xsd:", "", 4)}
        elif att_type == "file":
            return {"type": "file"}
        else:
            return self.get_simple_type(att_type)

    def get_doc_param(self, param_name, name, parent_name=None):
        """ get some parameters under configuration """
        if parent_name is not None:
            elem = self.get_attribute(name, parent_name)
        else:
            elem = self.get_element(name)

        if elem is None:
            return None

        param = elem.xpath("xsd:annotation/xsd:documentation%s" % param_name,
                           namespaces=NAMESPACES)
        if len(param) != 1:
            # search in references
            elem = self.get_reference(name)
            if elem is not None:
                param = elem.xpath("xsd:annotation/xsd:documentation%s" %
                                     param_name,
                                     namespaces=NAMESPACES)
            if len(param) != 1:
                return None

        return param[0].text

    def get_documentation(self, name, parent_name=None):
        """ get the description associated to an element """
        doc = self.get_doc_param('', name, parent_name)
        if doc is None:
            return None

        # remove indentation
        text = ' '.join(doc.split())
        # set tabulations and newlines
        text = text.replace('\\n', '\n').replace('\\t', '\t')
        return text

    def get_unit(self, name, parent_name=None):
        """ get the unit of an element """
        unit = self.get_doc_param('/unit', name, parent_name)
        if unit is None:
            return None

        # remove indentation
        text = ' '.join(unit.split())
        # set tabulations and newlines
        text = text.replace('\\n', '\n').replace('\\t', '\t')
        return text

    def get_file_source(self, name, parent_name=None):
        """ get the default and source parameters on file """
        return self.get_doc_param('/source', name, parent_name)

    def do_hide_adv(self, name, adv_mode):
        """ check if some widget should be hidden in non adv mode """
        # check if element should be hidden in non adv mode
        adv = self.get_doc_param('/adv', name)
        if adv is not None and not adv_mode and adv == "true":
            return True

        return False

    def do_hide(self, name):
        """ check if som widget should be hidden """
        # check if element has a hide parameter activated
        hide = self.get_doc_param('/hide', name)
        if hide is not None and hide == "true":
            return True

        return False

    def do_not_list(self, name):
        """ check if some widget shouldn't be displayed as a list """
        # check if element has a not_list attribute activated
        not_list = self.get_doc_param('/not_list', name)
        if not_list is not None and not_list == "true":
            return True

        return False

    def do_conf(self, name):
        """ check if some widget has to be configured """
        # check if element has a conf attribute activated
        conf = self.get_doc_param('/conf', name)
        if conf is not None and conf == "true":
            return True

        return False

    def get_xpath_restrictions(self, name, parent_name=None):
        """ get the unit of an element """
        if parent_name is not None:
            elem = self.get_attribute(name, parent_name)
        else:
            elem = self.get_element(name)

        if elem is None:
            return None

        param = elem.xpath("xsd:annotation/xsd:documentation/xpath",
                           namespaces=NAMESPACES)
        if len(param) != 1:
            # search in references
            elem = self.get_reference(name)
            if elem is not None:
                param = elem.xpath("xsd:annotation/xsd:documentation/xpath",
                                     namespaces=NAMESPACES)
            if len(param) != 1:
                return None

        return param[0].attrib

    def get_reference(self, name):
        """ get a reference in the XSD document """
        elem = []
        if self._common_xsd is not None:
            elem += self._common_xsd.xpath("//xsd:element[@ref = $val]",
                                           namespaces=NAMESPACES,
                                           val = name)
        elem += self._xsd_parser.xpath("//xsd:element[@ref = $val]",
                                       namespaces=NAMESPACES,
                                       val = name)
        if len(elem) == 0:
            return None

        # take the first element it should be better than nothing
        return elem[0]

    def get_element(self, name, with_type=False):
        """ get an element in the XSD document """
        elems = []
        if self._common_xsd is not None:
            elems += self._common_xsd.xpath("//xsd:element[@name = $val]",
                                            namespaces=NAMESPACES,
                                            val = name)
        elems += self._xsd_parser.xpath("//xsd:element[@name = $val]",
                                        namespaces=NAMESPACES,
                                        val = name)

        # sometimes there are 2 elements because debug keys got the same name,
        # take the first one with or without type
        if len(elems) == 1:
            return elems[0]
        elif len(elems) != 0:
            for elem in elems:
                if elem.get('type') is None and not with_type:
                    return elem
                elif elem.get('type') is not None and with_type:
                    return elem
            # return the first one, it should be better than nothing
            return elems[0]
        return None

    def get_attribute(self, name, parent_name):
        """ get an attribute in the XSD document """
        attribs = []
        if self._common_xsd is not None:
            attribs += self._common_xsd.xpath("//xsd:attribute[@name = $val]",
                                             namespaces=NAMESPACES,
                                             val = name)
        attribs += self._xsd_parser.xpath("//xsd:attribute[@name = $val]",
                                          namespaces=NAMESPACES,
                                          val = name)
        if attribs is None or len(attribs) == 0:
            return None

        # some elements can have the same attribute, get the attribute for the
        # desired element
        # first check if parent has a complex type
        cplx_name = self.get_complex_name(parent_name)
        if cplx_name is not None:
            parent_name = cplx_name
        for attrib in attribs:
            if attrib.getparent().getparent().get("name") == parent_name:
                return attrib

    def get_complex_name(self, name):
        """ get a completType element in the XSD document """
        elem = []
        if self._common_xsd is not None:
            elem += self._common_xsd.xpath("//xsd:element[@name = $val]",
                                           namespaces=NAMESPACES,
                                           val = name)
        elem += self._xsd_parser.xpath("//xsd:element[@name = $val]",
                                       namespaces=NAMESPACES,
                                       val = name)
        if len(elem) == 0:
            return None

        # we have now the type of the complex element in the type attribute
        # take the first element it should be better than nothingi
        cplx_name = elem[0].get('type')
        return cplx_name

    def get_simple_type(self, name):
        """ get a simple type and the associated attributes
            in a XSD document """
        # simpleType
        elems = []
        if self._common_xsd is not None:
            elems += self._common_xsd.xpath("//xsd:simpleType[@name = $val]",
                                            namespaces=NAMESPACES,
                                            val = name)
        elems += self._xsd_parser.xpath("//xsd:simpleType[@name = $val]",
                                        namespaces=NAMESPACES,
                                        val = name)
        if len(elems) == 0:
            return None

        # take the first element it should be better than nothing
        elem = elems[0]

        # restriction
        restrictions = elem.xpath("xsd:restriction",
                                  namespaces=NAMESPACES)
        if len(restrictions) != 1:
            return None

        restriction = restrictions[0]

        base = restriction.get("base")
        if base is None:
            return None

        ret = self.parse_numeric(base)
        if ret is not None:
            # get the minimum or maximum value if specified
            min_inc = restriction.xpath("xsd:minInclusive",
                                        namespaces=NAMESPACES)
            if len(min_inc) == 1:
                ret["min"] = min_inc[0].get("value")
            max_inc = restriction.xpath("xsd:maxInclusive",
                                        namespaces=NAMESPACES)
            if len(max_inc) == 1:
                ret['max'] = max_inc[0].get("value")
            fraction_digit = restriction.xpath("xsd:fractionDigits",
                                               namespaces=NAMESPACES)
            if len(fraction_digit) == 1:
                ret['step'] = "0.%s1" % ("0" *
                                         (int(fraction_digit[0].get("value")) -
                                          1))
            
            enum = restriction.xpath("xsd:enumeration",
                                     namespaces=NAMESPACES)
            values = []
            if enum is not None and len(enum) > 0:
                for elem in enum:
                    values.append(elem.get("value"))
            if len(values) != 0:
                ret['type'] = "enum"
                ret['enum'] = values

            return ret
        elif base == "xsd:string":
            enum = restriction.xpath("xsd:enumeration",
                                     namespaces=NAMESPACES)
            values = []
            if enum is not None and len(enum) > 0:
                for elem in enum:
                    values.append(elem.get("value"))
            if len(values) != 0:
                return {
                          "type": "enum",
                          "enum": values
                       }
            else:
                return {"type": "string"}
        else:
            return {"type": base.replace("xsd:", "", 4)}

    def get_minoccurs(self, table_name):
        """ get minOccurs value for table elements """
        values = []
        if self._common_xsd is not None:
            values += self._common_xsd.xpath("//xsd:element[@ref='%s']/@minOccurs" %
                                             table_name, namespaces=NAMESPACES)
        values += self._xsd_parser.xpath("//xsd:element[@ref='%s']/@minOccurs" %
                                         table_name, namespaces=NAMESPACES)
        if self._common_xsd is not None:
            values += self._common_xsd.xpath("//xsd:element[@name='%s']/@minOccurs" %
                                             table_name, namespaces=NAMESPACES)
        values += self._xsd_parser.xpath("//xsd:element[@name='%s']/@minOccurs" %
                                         table_name, namespaces=NAMESPACES)
        if len(values) > 0:
            # take the first element it should be better than nothing
            return int(values[0])
        else:
            return 0

    def get_maxoccurs(self, table_name):
        """ get maxOccurs value for table elements """
        values = []
        if self._common_xsd is not None:
            values +=  self._common_xsd.xpath("//xsd:element[@ref='%s']/@maxOccurs"
                                              % table_name, namespaces=NAMESPACES)
        values +=  self._xsd_parser.xpath("//xsd:element[@ref='%s']/@maxOccurs"
                                          % table_name, namespaces=NAMESPACES)
        if self._common_xsd is not None:
            values +=  self._common_xsd.xpath("//xsd:element[@name='%s']/@maxOccurs"
                                              % table_name, namespaces=NAMESPACES)
        values +=  self._xsd_parser.xpath("//xsd:element[@name='%s']/@maxOccurs"
                                          % table_name, namespaces=NAMESPACES)
        if len(values) > 0 and values[0] != "unbounded":
            # take the first element it should be better than nothing
            return int(values[0])
        elif len(values):
            # unbounded: return a high value
            return 100
        else:
            return 1

    def get_file_elements(self, elem):
        """ get elements or attributes with type: 'file' """
        values = []
        if self._common_xsd is not None:
            values +=  self._common_xsd.xpath("//xsd:%s[@type='file']/@name" %
                                              elem, namespaces=NAMESPACES)
        values +=  self._xsd_parser.xpath("//xsd:%s[@type='file']/@name" %
                                          elem, namespaces=NAMESPACES)
        return values

    def parse_numeric(self, base):
        """ parse a numeric xsd type """
        base = base.split(":", 1)
        if len(base) > 1:
            base = base[1]
        else:
            base = base[0]
        if base in ["byte",
                    "decimal",
                    "int",
                    "integer",
                    "long",
                    "negativeInteger",
                    "nonNegativeInteger",
                    "nonPositiveInteger",
                    "positiveInteger",
                    "short",
                    "unsignedLong",
                    "unsignedInt",
                    "unsignedShort",
                    "unsignedByte"]:
            min_val = None
            max_val = None
            step = 1
            # get the minimum or maximum value that can be infered
            if base in ["nonNegativeInteger",
                        "unsignedLong",
                        "unsignedInt",
                        "unsignedShort",
                        "unsignedByte"]:
                min_val = 0
            elif base == "nonPositiveInteger":
                max_val = 0
            elif base == "positiveInteger":
                min_val = 1
            elif base == "negativeInteger":
                max_val = -1
            elif base == "decimal":
                # arbitrarily fixed
                step = 0.01

            ret = {
                      "type": "numeric",
                      "step": step,
                   }
            if min_val is not None:
                ret["min"] = min_val
            if max_val is not None:
                ret["max"] = max_val
            return ret
        return None


    #### functions for XSLT transform ###

    def transform(self, xslt, write=True):
        """ Transform the configuration with XSLT """
        transf = etree.XSLT(etree.parse(xslt))
        new_xml = transf(self._tree)
        self._tree = new_xml
        if write:
            self.write()

if __name__ == "__main__":


    path_xsd = "xsd/topology.xsd"
    path_xml = "xml/topology.xml"

    
    xml_parser = XmlParser(path_xml,path_xsd)
    section = xml_parser.get_sections()[0]
    print(xml_parser.get_keys(section)[0].tag)




