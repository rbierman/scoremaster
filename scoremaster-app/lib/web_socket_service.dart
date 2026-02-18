import 'dart:convert';
import 'dart:async';
import 'package:web_socket_channel/web_socket_channel.dart';
import 'scoreboard_state.dart';
import 'team_model.dart';

enum ConnectionStatus { connected, disconnected, connecting }

class WebSocketService {
  WebSocketChannel? _channel;
  final _stateController = StreamController<ScoreboardState>.broadcast();
  final _teamsController = StreamController<List<Team>>.broadcast();
  final _imageController = StreamController<Map<String, dynamic>>.broadcast();
  final _connectionController = StreamController<ConnectionStatus>.broadcast();
  
  Stream<ScoreboardState> get stateStream => _stateController.stream;
  Stream<List<Team>> get teamsStream => _teamsController.stream;
  Stream<Map<String, dynamic>> get imageStream => _imageController.stream;
  Stream<ConnectionStatus> get connectionStream => _connectionController.stream;
  
  ConnectionStatus _status = ConnectionStatus.disconnected;
  ConnectionStatus get status => _status;

  String? _lastHost;
  int? _lastPort;
  bool _shouldReconnect = false;
  Timer? _reconnectTimer;

  Future<void> connect(String host, int port) async {
    _lastHost = host;
    _lastPort = port;
    _shouldReconnect = true;
    _doConnect();
  }

  void _updateStatus(ConnectionStatus status) {
    _status = status;
    _connectionController.add(status);
  }

  Future<void> _doConnect() async {
    if (_lastHost == null || _lastPort == null) return;
    
    _reconnectTimer?.cancel();
    _updateStatus(ConnectionStatus.connecting);
    
    final uri = Uri.parse('ws://$_lastHost:$_lastPort');
    
    try {
      _channel = WebSocketChannel.connect(uri);
      
      _channel!.stream.listen((message) {
        if (_status != ConnectionStatus.connected) {
          _updateStatus(ConnectionStatus.connected);
        }
        
        try {
          final Map<String, dynamic> data = jsonDecode(message);
          
          if (data.containsKey('type') && data['type'] == 'teams') {
            final List<dynamic> teamsJson = data['teams'];
            final teams = teamsJson.map((e) => Team.fromJson(e as Map<String, dynamic>)).toList();
            print('WebSocketService: Received ${teams.length} teams');
            _teamsController.add(teams);
          } else if (data.containsKey('type') && data['type'] == 'image') {
            _imageController.add(data);
          } else if (data.containsKey('homeScore')) { // Likely ScoreboardState
            final state = ScoreboardState.fromJson(data);
            _stateController.add(state);
          }
        } catch (e) {
          print('Error parsing message: $e');
        }
      }, onDone: () {
        _updateStatus(ConnectionStatus.disconnected);
        _scheduleReconnect();
      }, onError: (error) {
        _updateStatus(ConnectionStatus.disconnected);
        _scheduleReconnect();
      });
    } catch (e) {
      _updateStatus(ConnectionStatus.disconnected);
      _scheduleReconnect();
    }
  }

  void _scheduleReconnect() {
    if (!_shouldReconnect) return;
    
    _reconnectTimer?.cancel();
    _reconnectTimer = Timer(const Duration(seconds: 3), () {
      if (_status == ConnectionStatus.disconnected) {
        _doConnect();
      }
    });
  }

  void sendCommand(String command, {
    dynamic value, 
    int? delta, 
    int? index, 
    int? player, 
    int? minutes, 
    int? seconds,
    Map<String, dynamic>? extraArgs,
  }) {
    if (_channel == null || _status != ConnectionStatus.connected) return;
    
    final Map<String, dynamic> payload = {'command': command};
    if (value != null) payload['value'] = value;
    if (delta != null) payload['delta'] = delta;
    if (index != null) payload['index'] = index;
    if (player != null) payload['player'] = player;
    if (minutes != null) payload['minutes'] = minutes;
    if (seconds != null) payload['seconds'] = seconds;
    if (extraArgs != null) payload.addAll(extraArgs);

    try {
      _channel!.sink.add(jsonEncode(payload));
    } catch (e) {
      print('Failed to send command: $e');
    }
  }

  // Convenience methods
  void setHomeScore(int score) => sendCommand('setHomeScore', value: score);
  void setAwayScore(int score) => sendCommand('setAwayScore', value: score);
  void addHomeScore({int delta = 1}) => sendCommand('addHomeScore', delta: delta);
  void addAwayScore({int delta = 1}) => sendCommand('addAwayScore', delta: delta);
  void setHomeTeamName(String name) => sendCommand('setHomeTeamName', value: name);
  void setAwayTeamName(String name) => sendCommand('setAwayTeamName', value: name);
  
  void addOrUpdatePlayer(String teamName, String playerName, int playerNumber) {
    sendCommand('addOrUpdatePlayer', extraArgs: {
      'team': teamName,
      'name': playerName,
      'number': playerNumber
    });
  }

  void removePlayer(String teamName, int playerNumber) {
    sendCommand('removePlayer', extraArgs: {
      'team': teamName,
      'number': playerNumber
    });
  }

  void deleteTeam(String teamName) {
    sendCommand('deleteTeam', extraArgs: {'name': teamName});
  }

  void uploadPlayerImage(String teamName, int playerNumber, List<int> bytes) {
    sendCommand('uploadPlayerImage', extraArgs: {
      'team': teamName,
      'number': playerNumber,
      'data': base64Encode(bytes),
      'ext': '.jpg',
    });
  }

  void getPlayerImage(String teamName, int playerNumber) {
    sendCommand('getImage', extraArgs: {
      'team': teamName,
      'number': playerNumber,
    });
  }

  void getTeams() {
    sendCommand('getTeams');
  }

  void triggerGoal(bool isHome, {int? playerNumber}) {
    final Map<String, dynamic> args = {'isHome': isHome};
    if (playerNumber != null) {
      args['playerNumber'] = playerNumber;
    }
    sendCommand('triggerGoal', extraArgs: args);
  }

  void dispose() {
    _shouldReconnect = false;
    _reconnectTimer?.cancel();
    _channel?.sink.close();
    _stateController.close();
    _teamsController.close();
    _imageController.close();
    _connectionController.close();
  }
}
