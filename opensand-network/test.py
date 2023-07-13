
from opensand_xml_parser import XmlParser

if __name__ == "__main__":


    path_xsd = "xsd/topology.xsd"
    path_xml = "xml/topology.xml"
    xpath = "/model/root/frequency_plan/spots/item[1]/assignments/gateway_id"
    xpath_spot = "/model/root/frequency_plan/spots"

    xml_parser = XmlParser(path_xml,path_xsd)
    xml_parser.set_value(3,xpath)
    item_spot = xml_parser.get(xpath_spot)[0]
    
    xml_parser.add_element(xpath_spot+"/item",item_spot)

    xml_parser.write()




