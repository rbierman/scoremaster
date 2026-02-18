import 'dart:async';
import 'dart:convert';
import 'dart:io' show Platform;
import 'dart:typed_data';
import 'package:flutter/material.dart';
import 'package:image_picker/image_picker.dart';
import 'web_socket_service.dart';
import 'team_model.dart';

class TeamConfigurationPage extends StatefulWidget {
  final WebSocketService wsService;

  const TeamConfigurationPage({super.key, required this.wsService});

  @override
  State<TeamConfigurationPage> createState() => _TeamConfigurationPageState();
}

class _TeamConfigurationPageState extends State<TeamConfigurationPage> {
  List<Team> _teams = [];
  bool _isLoading = true;
  StreamSubscription? _teamsSubscription;
  StreamSubscription? _imageSubscription;
  final ImagePicker _picker = ImagePicker();
  
  // Cache for player images: "team_number" -> bytes
  final Map<String, Uint8List> _imageCache = {};

  @override
  void initState() {
    super.initState();
    _teamsSubscription = widget.wsService.teamsStream.listen((teams) {
      if (mounted) {
        setState(() {
          _teams = teams;
          _isLoading = false;
        });
        // Request images for players that have them and aren't in cache
        for (var team in teams) {
          for (var player in team.players) {
            String key = "${team.name}_${player.number}";
            if (player.hasImage && !_imageCache.containsKey(key)) {
              widget.wsService.getPlayerImage(team.name, player.number);
            }
          }
        }
      }
    });

    _imageSubscription = widget.wsService.imageStream.listen((imageData) {
      if (mounted) {
        String team = imageData['team'];
        int number = imageData['number'];
        String? base64 = imageData['data'];
        if (base64 != null) {
          setState(() {
            _imageCache["${team}_$number"] = base64Decode(base64);
          });
        }
      }
    });

    widget.wsService.getTeams();
  }

  @override
  void dispose() {
    _teamsSubscription?.cancel();
    _imageSubscription?.cancel();
    super.dispose();
  }

  Future<void> _pickImage(Team team, Player player) async {
    final source = Platform.isLinux ? ImageSource.gallery : ImageSource.camera;
    
    final XFile? photo = await _picker.pickImage(
      source: source,
      maxWidth: 400,
      maxHeight: 400,
      imageQuality: 85,
    );

    if (photo != null) {
      final bytes = await photo.readAsBytes();
      widget.wsService.uploadPlayerImage(team.name, player.number, bytes);
      
      // Optimistically update cache
      setState(() {
        _imageCache["${team.name}_${player.number}"] = bytes;
      });
    }
  }

  void _showAddTeamDialog() {
    final controller = TextEditingController();
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Add New Team'),
        content: TextField(
          controller: controller,
          decoration: const InputDecoration(labelText: 'Team Name'),
          autofocus: true,
        ),
        actions: [
          TextButton(onPressed: () => Navigator.pop(context), child: const Text('Cancel')),
          ElevatedButton(
            onPressed: () {
              if (controller.text.isNotEmpty) {
                widget.wsService.addOrUpdatePlayer(controller.text, "New Team", 0);
                Navigator.pop(context);
              }
            },
            child: const Text('Add'),
          ),
        ],
      ),
    );
  }

  void _showAddPlayerDialog(Team team) {
    final nameController = TextEditingController();
    final numberController = TextEditingController();
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: Text('Add Player to ${team.name}'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            TextField(
              controller: nameController,
              decoration: const InputDecoration(labelText: 'Player Name'),
              autofocus: true,
            ),
            TextField(
              controller: numberController,
              decoration: const InputDecoration(labelText: 'Number'),
              keyboardType: TextInputType.number,
            ),
          ],
        ),
        actions: [
          TextButton(onPressed: () => Navigator.pop(context), child: const Text('Cancel')),
          ElevatedButton(
            onPressed: () {
              int? num = int.tryParse(numberController.text);
              if (nameController.text.isNotEmpty && num != null) {
                widget.wsService.addOrUpdatePlayer(team.name, nameController.text, num);
                Navigator.pop(context);
              }
            },
            child: const Text('Add'),
          ),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Team Configuration'),
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: () {
              setState(() {
                _imageCache.clear();
              });
              widget.wsService.getTeams();
            },
          ),
        ],
      ),
      body: _isLoading 
        ? const Center(child: CircularProgressIndicator())
        : ListView.builder(
            itemCount: _teams.length,
            itemBuilder: (context, index) {
              final team = _teams[index];
              return Card(
                margin: const EdgeInsets.all(8.0),
                child: ExpansionTile(
                  title: Text(team.name, style: const TextStyle(fontWeight: FontWeight.bold, fontSize: 18)),
                  trailing: IconButton(
                    icon: const Icon(Icons.delete, color: Colors.red),
                    onPressed: () => _confirmDeleteTeam(team),
                  ),
                  children: [
                    ListView.builder(
                      shrinkWrap: true,
                      physics: const NeverScrollableScrollPhysics(),
                      itemCount: team.players.length,
                      itemBuilder: (context, pIndex) {
                        final player = team.players[pIndex];
                        if (player.number == 0 && player.name == "New Team") return const SizedBox.shrink();
                        
                        Uint8List? imageBytes = _imageCache["${team.name}_${player.number}"];

                        return ListTile(
                          leading: CircleAvatar(
                            backgroundImage: imageBytes != null ? MemoryImage(imageBytes) : null,
                            child: imageBytes == null ? Text('${player.number}') : null,
                          ),
                          title: Text(player.name),
                          subtitle: Text('#${player.number}'),
                          trailing: Row(
                            mainAxisSize: MainAxisSize.min,
                            children: [
                              IconButton(
                                icon: const Icon(Icons.camera_alt),
                                onPressed: () => _pickImage(team, player),
                              ),
                              IconButton(
                                icon: const Icon(Icons.remove_circle_outline, color: Colors.red),
                                onPressed: () => widget.wsService.removePlayer(team.name, player.number),
                              ),
                            ],
                          ),
                          onTap: () => _showEditPlayerDialog(team, player),
                        );
                      },
                    ),
                    Padding(
                      padding: const EdgeInsets.all(8.0),
                      child: ElevatedButton.icon(
                        onPressed: () => _showAddPlayerDialog(team),
                        icon: const Icon(Icons.add),
                        label: const Text('Add Player'),
                      ),
                    ),
                  ],
                ),
              );
            },
          ),
      floatingActionButton: FloatingActionButton(
        onPressed: _showAddTeamDialog,
        child: const Icon(Icons.group_add),
      ),
    );
  }

  void _showEditPlayerDialog(Team team, Player player) {
    final nameController = TextEditingController(text: player.name);
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: Text('Edit Player #${player.number}'),
        content: TextField(
          controller: nameController,
          decoration: const InputDecoration(labelText: 'Player Name'),
          autofocus: true,
        ),
        actions: [
          TextButton(onPressed: () => Navigator.pop(context), child: const Text('Cancel')),
          ElevatedButton(
            onPressed: () {
              if (nameController.text.isNotEmpty) {
                widget.wsService.addOrUpdatePlayer(team.name, nameController.text, player.number);
                Navigator.pop(context);
              }
            },
            child: const Text('Save'),
          ),
        ],
      ),
    );
  }

  void _confirmDeleteTeam(Team team) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Delete Team'),
        content: Text('Are you sure you want to delete the team "${team.name}"?'),
        actions: [
          TextButton(onPressed: () => Navigator.pop(context), child: const Text('Cancel')),
          ElevatedButton(
            onPressed: () {
              widget.wsService.deleteTeam(team.name);
              Navigator.pop(context);
            },
            style: ElevatedButton.styleFrom(backgroundColor: Colors.red, foregroundColor: Colors.white),
            child: const Text('Delete'),
          ),
        ],
      ),
    );
  }
}
