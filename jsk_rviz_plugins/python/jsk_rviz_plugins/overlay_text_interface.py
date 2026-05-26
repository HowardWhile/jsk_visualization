#!/usr/bin/env python3

from jsk_rviz_plugins.msg import OverlayText


def _private_topic(topic):
    if topic.startswith("~/"):
        return topic
    if topic.startswith("~"):
        return "~/" + topic[1:].lstrip("/")
    return topic


class OverlayTextInterface():
    def __init__(self, node, topic):
        self.node = node
        self.pub = node.create_publisher(OverlayText, _private_topic(topic), 10)
        self._declare_parameters()

    def _declare_parameters(self):
        defaults = {
            'width': 1200,
            'height': 800,
            'top': 10,
            'left': 10,
            'text_size': 12.0,
            'bg_red': 0.0,
            'bg_blue': 0.0,
            'bg_green': 0.0,
            'bg_alpha': 0.0,
            'fg_red': 25.0 / 255.0,
            'fg_blue': 1.0,
            'fg_green': 240.0 / 255.5,
            'fg_alpha': 1.0,
        }
        for name, value in defaults.items():
            self.node.declare_parameter(name, value)

    def _parameter(self, name):
        return self.node.get_parameter(name).value

    def publish(self, text):
        msg = OverlayText()
        msg.text = text
        msg.width = self._parameter('width')
        msg.height = self._parameter('height')
        msg.top = self._parameter('top')
        msg.left = self._parameter('left')
        msg.fg_color.a = self._parameter('fg_alpha')
        msg.fg_color.r = self._parameter('fg_red')
        msg.fg_color.g = self._parameter('fg_green')
        msg.fg_color.b = self._parameter('fg_blue')
        msg.bg_color.a = self._parameter('bg_alpha')
        msg.bg_color.r = self._parameter('bg_red')
        msg.bg_color.g = self._parameter('bg_green')
        msg.bg_color.b = self._parameter('bg_blue')
        msg.text_size = self._parameter('text_size')
        self.pub.publish(msg)
